/*****************************************************************************
* File:        main.c
* Description: T-box real-silicon CAN bring-up node -- NORMAL-mode FlexCAN over
*              the chosen canbus (CAN3 on the T-box, J33). ROLE_EDGE selects the
*              sender (1) or the receiver (0); both roles exchange sdv_telem_frame.
* Layer:       firmware/k3_tbox_silicon  (real-silicon 2-node CAN bench)
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

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
    .id = SDV_CAN_BASE_ID, .mask = 0x7F0, .flags = 0,   /* 0x100..0x10F */
};

int main(void)
{
    printk("2BOARD,boot,role=%s\n", ROLE_EDGE ? "edge" : "center");

    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    if (!device_is_ready(can_dev)) {
        printk("can,unavailable\n");
        return -1;
    }

    can_start(can_dev);
    can_add_rx_filter_msgq(can_dev, &rx_msgq, &telem_filter);
    printk("2BOARD,can,ok\n");

#if ROLE_EDGE
    uint16_t s = 0;
#endif
    while (1) {
#if ROLE_EDGE
        struct sdv_telem_frame out = {
            .node = SDV_NODE_POWERTRAIN, .signal = SIG_HEAP_FREE,
            .seq = s, .value = 8000u - s,
        };
        struct can_frame tx = {
            .id = SDV_CAN_BASE_ID | SDV_NODE_POWERTRAIN,
            .dlc = sizeof(out), .flags = 0,
        };
        memcpy(tx.data, &out, sizeof(out));
        int r = can_send(can_dev, &tx, K_MSEC(100), NULL, NULL);
        printk("EDGE,tx,seq=%u,ret=%d\n", s, r);   /* ret<0 = no ACK / bus not formed */
        s++;
        k_msleep(500);
#else
        struct can_frame frame;
        if (k_msgq_get(&rx_msgq, &frame, K_MSEC(1000)) == 0) {
            struct sdv_telem_frame *tf = (struct sdv_telem_frame *)frame.data;
            printk("CENTER,rx,id=%03x,node=%u,sig=%u,seq=%u,val=%u\n",
                   frame.id, tf->node, tf->signal, tf->seq, tf->value);
        } else {
            printk("CENTER,rx,waiting...\n");
        }
#endif
    }
    return 0;
}
