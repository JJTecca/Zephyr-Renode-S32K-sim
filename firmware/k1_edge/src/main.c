/*****************************************************************************
* File:        main.c
* Description: K1 edge node -- sense-only; TX heap telemetry on CAN. NEVER actuates.
* Layer:       firmware/k1_edge  (on-vehicle sense node)
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

/***************************************************
* INCLUDE FILES
***************************************************/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/sys_heap.h>
#include "telemetry.h"

/***************************************************
* MACRO DEFINITIONS
***************************************************/
#define TICK_MS       100
#define LEAK_HEAP_SZ  8192

/***************************************************
* STATIC DATA
***************************************************/
/* volatile: re-read each loop so Renode's mid-run writes are observed. */
static struct sdv_fault_ctl volatile sdv_fault_ctl;

K_HEAP_DEFINE(leak_heap, LEAK_HEAP_SZ);

static const struct device *can_dev;
static uint16_t seq;
static size_t total_leaked;

/***************************************************
* FUNCTION PROTOTYPES
***************************************************/
static void send_telem(uint8_t signal, uint32_t value);

/*****************************************************************************
* Function:    send_telem
* Description: Pack a telemetry frame and transmit it on CAN (id = 0x100 | node_id).
*****************************************************************************/
static void send_telem(uint8_t signal, uint32_t value)
{
    struct sdv_telem_frame tf = {
        .node   = CONFIG_SDV_NODE_ID,
        .signal = signal,
        .seq    = seq++,
        .value  = value,
    };
    struct can_frame frame = {
        .id   = SDV_CAN_BASE_ID | CONFIG_SDV_NODE_ID,
        .dlc  = sizeof(tf),
        .flags = 0,
    };
    __ASSERT_NO_MSG(sizeof(tf) <= CAN_MAX_DLEN);
    memcpy(frame.data, &tf, sizeof(tf));
    can_send(can_dev, &frame, K_MSEC(50), NULL, NULL);
}

/*****************************************************************************
* Function:    main
* Description: Boot, init CAN, then every 100 ms: fault-check, read heap, TX telemetry.
* Returns:     0 on normal exit; -1 if CAN is unavailable.
*****************************************************************************/
int main(void)
{
    printk("K1,boot,node=%d\n", CONFIG_SDV_NODE_ID);

    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    if (!device_is_ready(can_dev)) {
        printk("K1,err,can_not_ready\n");
        return -1;
    }

    int ret = can_start(can_dev);
    if (ret && ret != -EALREADY) {
        printk("K1,err,can_start=%d\n", ret);
        return -1;
    }

    printk("K1,can,ok\n");

    while (1) {
        /* Fault injection: leak memory if armed */
        if (sdv_fault_ctl.magic == SDV_FAULT_MAGIC &&
            sdv_fault_ctl.leak_bytes_per_tick > 0) {
            void *p = k_heap_alloc(&leak_heap,
                                   sdv_fault_ctl.leak_bytes_per_tick,
                                   K_NO_WAIT);
            if (p) {
                total_leaked += sdv_fault_ctl.leak_bytes_per_tick;
            }
        }

        /* Read heap stats and TX telemetry */
        struct sys_memory_stats stats;
        sys_heap_runtime_stats_get(&leak_heap.heap, &stats);

        send_telem(SIG_HEAP_FREE, (uint32_t)stats.free_bytes);
        send_telem(SIG_HEAP_USED, (uint32_t)stats.allocated_bytes);

        k_msleep(TICK_MS);
    }
    return 0;
}
