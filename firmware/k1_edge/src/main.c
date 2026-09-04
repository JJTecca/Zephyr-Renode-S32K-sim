/*****************************************************************************
* File:        main.c
* Description: K1 edge node -- sense-only; streams heap + loop-latency telemetry.
*              NEVER actuates. Emits on console UART, CAN-when-up, and the
*              inter-node link UART (lpuart1, ADR-017). Two injectable faults:
*              memory-leak (accumulating) and timing/deadline-miss (ramping).
* Layer:       firmware/k1_edge (L1 sense + inject)
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/sys_heap.h>
#include "telemetry.h"
#include "actions.h"

#define TICK_MS       100
#define LEAK_HEAP_SZ  8192
#define BUSY_CAP_US   500000u   /* stalling threshold because we dont want to go higher */
#define STAGGER_MS    25        /* senders don't collide */

static struct sdv_fault_ctl volatile sdv_fault_ctl;

K_HEAP_DEFINE(leak_heap, LEAK_HEAP_SZ);

static const struct device *can_dev;
static const struct device *link_uart;   /* inter-node telemetry bus (lpuart1) */
static bool     can_ok;
static uint16_t seq;
static size_t   total_leaked;
static uint32_t busy_accum_us;            /* ramping deadline-miss injection */

/* Emit one telemetry record as a text line on the inter-node link UART:
 * "L,node,signal,seq,value\n" -- K3 parses these (ADR-017 transport). */
static void link_emit(uint8_t signal, uint32_t value)
{
    char buf[48];
    int n = snprintk(buf, sizeof(buf), "L,%u,%u,%u,%u\n",
                     (unsigned)CONFIG_SDV_NODE_ID, signal, seq, value);
    for (int i = 0; i < n; i++) {
        uart_poll_out(link_uart, buf[i]);
    }
    printk("K1,tx,link,node=%u,sig=%u,seq=%u,val=%u\n",
           (unsigned)CONFIG_SDV_NODE_ID, signal, seq, value);
}

static void send_telem(uint8_t signal, uint32_t value)
{
    printk("TELEM,%lld,%u,%u,%u,%u\n",
           k_uptime_get(), (unsigned)CONFIG_SDV_NODE_ID, signal, seq, value);

    if (link_uart) {
        link_emit(signal, value);
    }

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

int main(void)
{
    printk("K1,boot,node=%d\n", CONFIG_SDV_NODE_ID);

    /* Inter-node link UART (lpuart1): the sim's stand-in bus (ADR-017). */
    link_uart = DEVICE_DT_GET(DT_NODELABEL(lpuart1));
    if (device_is_ready(link_uart)) {
        printk("K1,link,ok\n");
    } else {
        link_uart = NULL;
        printk("K1,link,unavailable\n");
    }

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

    /* #define CONFIG_SDV_NODE_ID 1 */
    k_msleep((CONFIG_SDV_NODE_ID - 1) * STAGGER_MS);
    while (1) {
        int64_t t0 = k_uptime_get();
        bool armed = (sdv_fault_ctl.magic == SDV_FAULT_MAGIC);

        /* Fault 1: memory leak (accumulating). */
        if (armed && sdv_fault_ctl.leak_bytes_per_tick > 0) {
            void *p = k_heap_alloc(&leak_heap,
                                   sdv_fault_ctl.leak_bytes_per_tick, K_NO_WAIT);
            if (p) {
                total_leaked += sdv_fault_ctl.leak_bytes_per_tick;
            }
        }

        /* Fault 2: timing / deadline-miss (ramping extra work per tick). */
        if (armed && sdv_fault_ctl.busy_spin_us > 0) {
            busy_accum_us += sdv_fault_ctl.busy_spin_us;
            if (busy_accum_us > BUSY_CAP_US) {
                busy_accum_us = BUSY_CAP_US;
            }
            k_busy_wait(busy_accum_us);
        } else {
            busy_accum_us = 0;   /* clearing the fault heals: latency drops */
        }

        struct sys_memory_stats stats;
        sys_heap_runtime_stats_get(&leak_heap.heap, &stats);
        send_telem(SIG_HEAP_FREE, (uint32_t)stats.free_bytes);
        send_telem(SIG_HEAP_USED, (uint32_t)stats.allocated_bytes);

        /* calculate working delta */
        uint32_t work_ms = (uint32_t)(k_uptime_get() - t0);
        printk("Delta work_ms = %u \n",work_ms);
        send_telem(SIG_LOOP_LATENCY, work_ms);

        k_msleep(TICK_MS);
    }
    return 0;
}