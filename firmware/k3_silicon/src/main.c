#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <string.h>
#include "telemetry.h"

#ifndef ROLE_EDGE
#define ROLE_EDGE 1
#endif

static const struct device *can_dev;
CAN_MSGQ_DEFINE(rx_msgq, 16);
static const struct can_filter telem_filter = {
	.id = SDV_CAN_BASE_ID, .mask = 0x7F0, .flags = 0,
};

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

static void dump(const char *tag)
{
	enum can_state s; struct can_bus_err_cnt ec;
	can_get_state(can_dev, &s, &ec);
	printk("%s,state=%s,tec=%u,rec=%u\n", tag, st(s), ec.tx_err_cnt, ec.rx_err_cnt);
}

#if ROLE_EDGE
static void tx_done(const struct device *dev, int error, void *arg)
{
	ARG_UNUSED(dev); ARG_UNUSED(arg);
	printk("EDGE,tx_done,err=%d\n", error);   /* 0=ACKed, <0=no ACK */
}
#endif

int main(void)
{
	printk("2BOARD,boot,role=%s\n", ROLE_EDGE ? "edge" : "center");
	can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	if (!device_is_ready(can_dev)) { printk("can,unavailable\n"); return -1; }
	int ret = can_start(can_dev);
	if (ret != 0 && ret != -EALREADY) { printk("2BOARD,can,start_err=%d\n", ret); return -1; }
	can_add_rx_filter_msgq(can_dev, &rx_msgq, &telem_filter);
	printk("2BOARD,can,ok\n");
	dump("2BOARD,after_start");

#if ROLE_EDGE
	uint16_t sq = 0;
#endif
	while (1) {
#if ROLE_EDGE
		struct sdv_telem_frame out = {
			.node = SDV_NODE_POWERTRAIN, .signal = SIG_HEAP_FREE,
			.seq = sq, .value = 8000u - sq,
		};
		struct can_frame tx = {
			.id = SDV_CAN_BASE_ID | SDV_NODE_POWERTRAIN,
			.dlc = sizeof(out), .flags = 0,
		};
		memcpy(tx.data, &out, sizeof(out));
		int r = can_send(can_dev, &tx, K_MSEC(100), tx_done, NULL);
		printk("EDGE,seq=%u,enq=%d\n", sq, r);
		dump("EDGE");
		sq++;
		k_msleep(500);
#else
		struct can_frame frame;
		if (k_msgq_get(&rx_msgq, &frame, K_MSEC(1000)) == 0) {
			struct sdv_telem_frame *tf = (struct sdv_telem_frame *)frame.data;
			printk("CENTER,rx,id=%03x,node=%u,sig=%u,seq=%u,val=%u\n",
			       frame.id, tf->node, tf->signal, tf->seq, tf->value);
		} else {
			dump("CENTER,waiting");
		}
#endif
	}
	return 0;
}