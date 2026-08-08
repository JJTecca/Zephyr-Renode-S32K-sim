/*****************************************************************************
* File:        main.c
* Description: K1 edge node -- sense-only; streams heap telemetry. NEVER actuates.
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
static bool     can_ok;          /* true once the CAN controller is running */
static uint16_t seq;
static size_t   total_leaked;

/***************************************************
* FUNCTION PROTOTYPES
***************************************************/
static void send_telem(uint8_t signal, uint32_t value);

/*****************************************************************************
* Function:    send_telem
* Description: Emit one sample. Always prints a CSV line on UART (the sim
*              dataset stream); also TX on CAN when the bus is up (silicon).
*****************************************************************************/
static void send_telem(uint8_t signal, uint32_t value)
{
    printk("TELEM,%lld,%u,%u,%u,%u\n",
           k_uptime_get(), (unsigned)CONFIG_SDV_NODE_ID, signal, seq, value);

    if (can_ok) {
        struct sdv_telem_frame tf = {
            .node = CONFIG_SDV_NODE_ID, .signal = signal,
            .seq = seq, .value = value,
        };
        struct can_frame frame = {
            .id = SDV_CAN_BASE_ID | CONFIG_SDV_NODE_ID,
            .dlc = sizeof(tf), .flags = 0,
        };
        memcpy(frame.data, &tf, sizeof(tf));
        can_send(can_dev, &frame, K_MSEC(50), NULL, NULL);
    }

    seq++;
}

/*****************************************************************************
* Function:    main
* Description: Boot, bring up CAN if available, then every 100 ms:
*              fault-check, read heap, stream telemetry.
* Returns:     0 (does not return in practice).
*****************************************************************************/
int main(void)
{
    printk("K1,boot,node=%d\n", CONFIG_SDV_NODE_ID);

    /* CAN is optional in sim: the Renode S32K388 model doesn't drive the
     * FlexCAN clock, so init fails here -- not fatal, UART still streams.
     * On real silicon the clock is real and CAN comes up normally. */
    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    if (device_is_ready(can_dev)) {
        int ret = can_start(can_dev);
        if (ret == 0 || ret == -EALREADY) {
            can_ok = true;
            printk("K1,can,ok\n");
        } else {
            printk("K1,can,start_err=%d\n", ret);
        }
    } else {
        printk("K1,can,unavailable_sim\n");
    }

    while (1) {
        /* Fault injection: leak if armed by Renode in virtual time. */
        if (sdv_fault_ctl.magic == SDV_FAULT_MAGIC &&
            sdv_fault_ctl.leak_bytes_per_tick > 0) {
            void *p = k_heap_alloc(&leak_heap,
                                   sdv_fault_ctl.leak_bytes_per_tick,
                                   K_NO_WAIT);
            if (p) {
                total_leaked += sdv_fault_ctl.leak_bytes_per_tick;
            }
        }

        struct sys_memory_stats stats;
        sys_heap_runtime_stats_get(&leak_heap.heap, &stats);

        send_telem(SIG_HEAP_FREE, (uint32_t)stats.free_bytes);
        send_telem(SIG_HEAP_USED, (uint32_t)stats.allocated_bytes);

        k_msleep(TICK_MS);
    }
    return 0;
}