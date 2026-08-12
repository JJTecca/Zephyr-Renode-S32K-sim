/*****************************************************************************
* File:        main.c
* Description: K1 edge node -- sense-only; streams heap telemetry. NEVER actuates.
*              Telemetry goes out three ways: console UART (dataset stream),
*              CAN when the bus is up (silicon), and the inter-node link UART
*              (lpuart1) -- the Renode UART hub that stands in for CAN FD in
*              simulation (ADR-017).
* Layer:       firmware/k1_edge  (on-vehicle sense node)
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/sys_heap.h>
#include "telemetry.h"

#define TICK_MS       100
#define LEAK_HEAP_SZ  8192

static struct sdv_fault_ctl volatile sdv_fault_ctl;

K_HEAP_DEFINE(leak_heap, LEAK_HEAP_SZ);

static const struct device *can_dev;
static const struct device *link_uart;   /* inter-node telemetry bus (lpuart1) */
static bool     can_ok;
static uint16_t seq;
static size_t   total_leaked;

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

    while (1) {
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