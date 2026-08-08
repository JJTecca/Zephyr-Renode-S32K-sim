/*****************************************************************************
* File:        main.c
* Description: K3 zonal hub -- RX telemetry on CAN, print CSV on the console.
* Layer:       firmware/k3_hub  (on-vehicle loop host)
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

/***************************************************
* INCLUDE FILES
***************************************************/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include "telemetry.h"

/***************************************************
* STATIC DATA
***************************************************/
static const struct device *can_dev;

/* RX queue + filter: match the telemetry ID block 0x100-0x10F
 * (mask 0x7F0 ignores the low 4 node-id bits). */
CAN_MSGQ_DEFINE(rx_msgq, 16);
static const struct can_filter telem_filter = {
    .id    = SDV_CAN_BASE_ID,   /* 0x100 */
    .mask  = 0x7F0,
    .flags = 0,
};

/*****************************************************************************
* Function:    main
* Description: Boot, init CAN, then block on RX and print each telemetry frame.
* Returns:     0 on normal exit; -1 if CAN is unavailable.
*****************************************************************************/
int main(void)
{
    printk("K3,boot,ok\n");

    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    // CAN not ready is ok for simulation
    if (!device_is_ready(can_dev)) {
        printk("K3,err,can_not_ready\n");
        return -1;
    }

    int ret = can_start(can_dev);
    if (ret && ret != -EALREADY) {
        printk("K3,err,can_start=%d\n", ret);
        return -1;
    }

    /* can.h function */
    can_add_rx_filter_msgq(can_dev, &rx_msgq, &telem_filter);
    printk("K3,can,ok\n");

    while (1) {
        struct can_frame frame;

        /* BLOCK until a telemetry frame arrives */
        if (k_msgq_get(&rx_msgq, &frame, K_FOREVER) == 0) {
            struct sdv_telem_frame *tf = (struct sdv_telem_frame *)frame.data;
            printk("TELEM,%lld,%u,%u,%u,%u\n",
                   k_uptime_get(), tf->node, tf->signal, tf->seq, tf->value);
        }
    }
    return 0;
}