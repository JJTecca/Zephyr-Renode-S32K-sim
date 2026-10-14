#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <string.h>
#include "telemetry.h"
#include "config.h"
#include "actions.h"
#if !ROLE_EDGE
#include "ae_model.h"
#endif

#ifndef ROLE_EDGE
#define ROLE_EDGE 1
#endif
#ifndef SDV_NODE
#define SDV_NODE SDV_NODE_POWERTRAIN
#endif

struct __attribute__((packed)) sdv_cmd_frame { uint8_t node; uint8_t action; };

static const struct device *can_dev;

static const char *st(enum can_state s)
{
	switch (s) {
	case CAN_STATE_ERROR_ACTIVE:  return "err-active";
	case CAN_STATE_ERROR_WARNING: return "err-warn";
	case CAN_STATE_ERROR_PASSIVE: return "err-passive";
	case CAN_STATE_BUS_OFF:       return "BUS-OFF";
	case CAN_STATE_STOPPED:       return "stopped";
	default:                      return "?";
	}
}

#if ROLE_EDGE
CAN_MSGQ_DEFINE(cmdq, 4);
static const struct can_filter cmd_flt = { .id = SDV_CAN_CMD_ID, .mask = 0x7F0, .flags = 0 };

static struct sdv_fault_ctl volatile sdv_fault_ctl;
static char leak_buf[LEAK_HEAP_SZ] __aligned(8);
static struct k_heap leak_heap;
static uint16_t seq;
static uint32_t busy_accum_us;
static size_t   total_leaked;
static bool     degraded;
static void tx_cb(const struct device *dev, int err, void *arg) { ARG_UNUSED(dev); ARG_UNUSED(err); ARG_UNUSED(arg); }

static void send_telem(uint8_t sig, uint32_t val)
{
	printk("TELEM,%lld,%u,%u,%u,%u\n", k_uptime_get(), (unsigned)SDV_NODE, sig, seq, val);
	struct sdv_telem_frame tf = { .node = SDV_NODE, .signal = sig, .seq = seq, .value = val };
	struct can_frame f = { .id = SDV_CAN_BASE_ID | SDV_NODE, .dlc = sizeof(tf), .flags = 0 };
	memcpy(f.data, &tf, sizeof(tf));
	can_send(can_dev, &f, K_NO_WAIT, tx_cb, NULL);
	seq++;
}

static void apply_cmd(int action)
{
	if (action == SDV_RESTART) {
		sdv_fault_ctl.magic = 0;
		total_leaked = 0;
		k_heap_init(&leak_heap, leak_buf, LEAK_HEAP_SZ);   /* reclaim -> heap_free recovers */
		degraded = false;
	} else if (action == SDV_DEGRADED_MODE || action == SDV_LOAD_SHED) {
		sdv_fault_ctl.magic = 0;                           /* stop the fault source */
		degraded = true;
	}
	send_telem(SIG_ACTION_TAKEN, (uint32_t)action);
	printk("EDGE,heal,action=%d\n", action);
}
#endif

#if !ROLE_EDGE
CAN_MSGQ_DEFINE(rxq, 16);
static const struct can_filter flt = { .id = SDV_CAN_BASE_ID, .mask = 0x7F0, .flags = 0 };

#define AE_WIN      10
#define AE_DT_S     0.1f
#define AE_W1       (AE_WIN + 1)
#define TTF_GUARD_S 3.0f

static float    hf_hist[AE_W1], ll_hist[AE_W1];
static uint32_t det_tick, guard_blocks, latched_hf;
static bool     have_hf, acting;

static int action_allowed(int a) { return a == SDV_RESTART || a == SDV_DEGRADED_MODE || a == SDV_LOAD_SHED; }

static void send_cmd(int node, int action)
{
	struct sdv_cmd_frame c = { .node = (uint8_t)node, .action = (uint8_t)action };
	struct can_frame f = { .id = SDV_CAN_CMD_ID | node, .dlc = sizeof(c), .flags = 0 };
	memcpy(f.data, &c, sizeof(c));
	can_send(can_dev, &f, K_MSEC(10), NULL, NULL);
	printk("K3,tx,cmd,node=%d,action=%d\n", node, action);
}

static int supervisor_step(int src, int alarm, float ttf, int proposed)
{
	if (!alarm) return SDV_NONE;
	if (!action_allowed(proposed)) {
		guard_blocks++;
		printk("K3,supervisor,block,src=%d,action=%d,reason=whitelist\n", src, proposed);
		printk("K3,supervisor,guard_blocks=%u\n", guard_blocks);
		return SDV_NONE;
	}
	if (proposed == SDV_RESTART && ttf > TTF_GUARD_S) {
		guard_blocks++;
		printk("K3,supervisor,block,src=%d,action=RESTART,reason=ttf%.1f\n", src, (double)ttf);
		printk("K3,supervisor,guard_blocks=%u\n", guard_blocks);
		return SDV_NONE;
	}
	if (acting) return SDV_NONE;
	acting = true;
	printk("K3,supervisor,approve,src=%d,action=%d,ttf=%.1f\n", src, proposed, (double)ttf);
	return proposed;
}

static void dense(const float *w, const float *b, const float *in, float *out, int no, int ni, int relu)
{
	for (int o = 0; o < no; o++) {
		float acc = b[o];
		for (int i = 0; i < ni; i++) acc += w[o * ni + i] * in[i];
		out[o] = (relu && acc < 0.f) ? 0.f : acc;
	}
}

static float ae_score(const float x[AE_D0])
{
	float z[AE_D0], h1[AE_D1], h2[AE_D2], h3[AE_D3], o[AE_D4];
	for (int i = 0; i < AE_D0; i++) z[i] = (x[i] - ae_mu[i]) / ae_sd[i];
	dense((const float *)ae_w0, ae_b0, z,  h1, AE_D1, AE_D0, 1);
	dense((const float *)ae_w1, ae_b1, h1, h2, AE_D2, AE_D1, 1);
	dense((const float *)ae_w2, ae_b2, h2, h3, AE_D3, AE_D2, 1);
	dense((const float *)ae_w3, ae_b3, h3, o,  AE_D4, AE_D3, 0);
	float mse = 0.f;
	for (int i = 0; i < AE_D4; i++) { float d = o[i] - z[i]; mse += d * d; }
	return mse / AE_D4;
}

static void detector_step(uint16_t seq, uint32_t heap_free, uint32_t loop_latency)
{
	float hf = (float)heap_free, ll = (float)loop_latency;
	hf_hist[det_tick % AE_W1] = hf;
	ll_hist[det_tick % AE_W1] = ll;
	uint32_t n   = det_tick < AE_WIN ? det_tick : AE_WIN;
	uint32_t old = (det_tick - n) % AE_W1;
	float hf_slope = n ? (hf - hf_hist[old]) / (n * AE_DT_S) : 0.f;
	float ll_slope = n ? (ll - ll_hist[old]) / (n * AE_DT_S) : 0.f;
	det_tick++;

	float x[AE_D0] = { hf, hf_slope, ll, ll_slope };
	uint32_t c0 = k_cycle_get_32();
	float s = ae_score(x);
	uint32_t infer_ns = (uint32_t)k_cyc_to_ns_floor64(k_cycle_get_32() - c0);  /* on-board cost (D7) */

	int alarm = s > AE_THRESHOLD;
	int whole = (int)s, milli = (int)((s - (float)whole) * 1000.f);
	printk("K3,score,seq=%u,score=%d.%03d,alarm=%d,infer_ns=%u\n", seq, whole, milli, alarm, infer_ns);

	if (alarm) {
		float drain = -hf_slope; if (drain < 1e-6f) drain = 1e-6f;
		float ttf = heap_free / drain;
		int act = supervisor_step(SDV_NODE_POWERTRAIN, alarm, ttf, SDV_RESTART);
		if (act != SDV_NONE) send_cmd(SDV_NODE_POWERTRAIN, act);
	} else if (acting) {
		acting = false;
		printk("K3,supervisor,rearm\n");
	}
}
#endif

int main(void)
{
	printk("2BOARD,boot,role=%s,node=%u\n", ROLE_EDGE ? "edge" : "center", (unsigned)SDV_NODE);
	can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	if (!device_is_ready(can_dev)) { printk("can,unavailable\n"); return -1; }
	int r = can_start(can_dev);
	if (r != 0 && r != -EALREADY) { printk("2BOARD,can,start_err=%d\n", r); return -1; }
	printk("2BOARD,can,ok\n");

#if ROLE_EDGE
	can_add_rx_filter_msgq(can_dev, &cmdq, &cmd_flt);
	k_heap_init(&leak_heap, leak_buf, LEAK_HEAP_SZ);
	uint32_t tick = 0;
	while (1) {
		struct can_frame cf;
		if (k_msgq_get(&cmdq, &cf, K_NO_WAIT) == 0) {
			struct sdv_cmd_frame *c = (struct sdv_cmd_frame *)cf.data;
			if (c->node == SDV_NODE) apply_cmd(c->action);
		}
		int64_t t0 = k_uptime_get();
		bool armed = (sdv_fault_ctl.magic == SDV_FAULT_MAGIC);

		if (armed && sdv_fault_ctl.leak_bytes_per_tick > 0) {
			void *p = k_heap_alloc(&leak_heap, sdv_fault_ctl.leak_bytes_per_tick, K_NO_WAIT);
			if (p) total_leaked += sdv_fault_ctl.leak_bytes_per_tick;
		}
		if (armed && sdv_fault_ctl.busy_spin_us > 0) {
			busy_accum_us += sdv_fault_ctl.busy_spin_us;
			if (busy_accum_us > BUSY_CAP_US) busy_accum_us = BUSY_CAP_US;
			k_busy_wait(busy_accum_us);
		} else {
			busy_accum_us = 0;
		}

		send_telem(SIG_HEAP_FREE, (uint32_t)(LEAK_HEAP_SZ - total_leaked));
		send_telem(SIG_HEAP_USED, (uint32_t)total_leaked);
		send_telem(SIG_LOOP_LATENCY, (uint32_t)(k_uptime_get() - t0));

		if (++tick % 10 == 0) {
			enum can_state s; struct can_bus_err_cnt ec;
			can_get_state(can_dev, &s, &ec);
			printk("EDGE,health,state=%s,tec=%u,rec=%u,degraded=%d\n", st(s), ec.tx_err_cnt, ec.rx_err_cnt, degraded);
		}
		k_msleep(TICK_MS);
	}
#else
	can_add_rx_filter_msgq(can_dev, &rxq, &flt);
	struct can_frame fr;
	while (1) {
		if (k_msgq_get(&rxq, &fr, K_MSEC(1000)) == 0) {
			struct sdv_telem_frame *tf = (struct sdv_telem_frame *)fr.data;
			printk("TELEM,%lld,%u,%u,%u,%u\n", k_uptime_get(), tf->node, tf->signal, tf->seq, tf->value);
			if (tf->node == SDV_NODE_POWERTRAIN) {
				if (tf->signal == SIG_HEAP_FREE) { latched_hf = (uint32_t)tf->value; have_hf = true; }
				else if (tf->signal == SIG_LOOP_LATENCY && have_hf) {
					detector_step(tf->seq, latched_hf, (uint32_t)tf->value);
					have_hf = false;
				}
			}
		} else {
			enum can_state s; struct can_bus_err_cnt ec;
			can_get_state(can_dev, &s, &ec);
			printk("CENTER,waiting,state=%s,tec=%u,rec=%u\n", st(s), ec.tx_err_cnt, ec.rx_err_cnt);
		}
	}
#endif
	return 0;
}