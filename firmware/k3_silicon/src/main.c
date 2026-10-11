#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <string.h>
#include "telemetry.h"

#ifndef ROLE_EDGE
#define ROLE_EDGE 1
#endif
#ifndef SDV_NODE
#define SDV_NODE SDV_NODE_POWERTRAIN
#endif

#define TICK_MS      100
#define LEAK_HEAP_SZ 8192
#define BUSY_CAP_US  500000u

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

#if !ROLE_EDGE
CAN_MSGQ_DEFINE(rxq, 16);
static const struct can_filter flt = { .id = SDV_CAN_BASE_ID, .mask = 0x7F0, .flags = 0 };
#endif

#if ROLE_EDGE
/* Fixed-symbol fault block: Renode or the S32DS debugger arms it to inject. */
static struct sdv_fault_ctl volatile sdv_fault_ctl;
static char leak_buf[LEAK_HEAP_SZ] __aligned(8);
static struct k_heap leak_heap;
static uint16_t seq;
static uint32_t busy_accum_us;
static size_t   total_leaked;

static void send_telem(uint8_t sig, uint32_t val)
{
	printk("TELEM,%lld,%u,%u,%u,%u\n", k_uptime_get(), (unsigned)SDV_NODE, sig, seq, val);
	struct sdv_telem_frame tf = { .node = SDV_NODE, .signal = sig, .seq = seq, .value = val };
	struct can_frame f = { .id = SDV_CAN_BASE_ID | SDV_NODE, .dlc = sizeof(tf), .flags = 0 };
	memcpy(f.data, &tf, sizeof(tf));
	can_send(can_dev, &f, K_MSEC(50), NULL, NULL);
	seq++;
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
	k_heap_init(&leak_heap, leak_buf, LEAK_HEAP_SZ);
	uint32_t tick = 0;
	while (1) {
		int64_t t0 = k_uptime_get();
		bool armed = (sdv_fault_ctl.magic == SDV_FAULT_MAGIC);

		/* Fault 1: memory leak (accumulating, real allocation). */
		if (armed && sdv_fault_ctl.leak_bytes_per_tick > 0) {
			void *p = k_heap_alloc(&leak_heap, sdv_fault_ctl.leak_bytes_per_tick, K_NO_WAIT);
			if (p) {
				total_leaked += sdv_fault_ctl.leak_bytes_per_tick;
			}
		}

		/* Fault 2: timing / deadline-miss (ramping extra work per tick). */
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
			printk("EDGE,health,state=%s,tec=%u,rec=%u\n", st(s), ec.tx_err_cnt, ec.rx_err_cnt);
		}
		k_msleep(TICK_MS);
	}
#else
	can_add_rx_filter_msgq(can_dev, &rxq, &flt);
	struct can_frame fr;
	while (1) {
		if (k_msgq_get(&rxq, &fr, K_MSEC(1000)) == 0) {
			struct sdv_telem_frame *tf = (struct sdv_telem_frame *)fr.data;
			printk("TELEM,%lld,%u,%u,%u,%u\n", k_uptime_get(),
			       tf->node, tf->signal, tf->seq, tf->value);
		} else {
			enum can_state s; struct can_bus_err_cnt ec;
			can_get_state(can_dev, &s, &ec);
			printk("CENTER,waiting,state=%s,tec=%u,rec=%u\n", st(s), ec.tx_err_cnt, ec.rx_err_cnt);
		}
	}
#endif
	return 0;
}