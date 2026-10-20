/*****************************************************************************
* File:        main.c
* Description: K3 node on real silicon -- edge injects faults, streams
*              telemetry over CAN and actuates heal commands; center receives
*              and runs the shared detector + supervisor (sdv_detect.c).
* Layer:       firmware/k3_silicon
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <string.h>
#include "telemetry.h"
#include "actions.h"
#include "sdv_detect.h"

#ifndef ROLE_EDGE
#define ROLE_EDGE 1
#endif
#ifndef SDV_NODE
#define SDV_NODE SDV_NODE_POWERTRAIN
#endif

#define TICK_MS        100
#define LEAK_HEAP_SZ   65536
#define BUSY_CAP_US    500000u
#define SDV_CAN_CMD_ID 0x200

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

void sdv_send_cmd(int node, int action)
{
	struct sdv_cmd_frame c = { .node = (uint8_t)node, .action = (uint8_t)action };
	struct can_frame f = { .id = SDV_CAN_CMD_ID | node, .dlc = sizeof(c), .flags = 0 };
	memcpy(f.data, &c, sizeof(c));
	can_send(can_dev, &f, K_MSEC(10), NULL, NULL);
	printk("K3,tx,cmd,node=%d,action=%d\n", node, action);
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
		k_heap_init(&leak_heap, leak_buf, LEAK_HEAP_SZ);
		degraded = false;
	} else if (action == SDV_DEGRADED_MODE || action == SDV_LOAD_SHED) {
		sdv_fault_ctl.magic = 0;
		degraded = true;
	}
	send_telem(SIG_ACTION_TAKEN, (uint32_t)action);
	printk("EDGE,heal,action=%d\n", action);
}
#endif

#if !ROLE_EDGE
CAN_MSGQ_DEFINE(rxq, 16);
static const struct can_filter flt = { .id = SDV_CAN_BASE_ID, .mask = 0x7F0, .flags = 0 };
static uint32_t latched_hf[SDV_MAX_NODES];
static bool     have_hf[SDV_MAX_NODES];
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
			if (tf->node < SDV_MAX_NODES) {
				if (tf->signal == SIG_HEAP_FREE) { latched_hf[tf->node] = (uint32_t)tf->value; have_hf[tf->node] = true; }
				else if (tf->signal == SIG_LOOP_LATENCY && have_hf[tf->node]) {
					detector_step(tf->node, tf->seq, latched_hf[tf->node], (uint32_t)tf->value);
					have_hf[tf->node] = false;
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