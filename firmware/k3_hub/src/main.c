/*****************************************************************************
* File:        main.c
* Description: K3 zonal hub -- gathers edge telemetry and runs the shared
*              on-vehicle detector + supervisor (sdv_detect.c).
* Layer:       firmware/k3_hub
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include "telemetry.h"
#include "actions.h"
#include "sdv_detect.h"

static const struct device *can_dev;
static const struct device *link_uart;

CAN_MSGQ_DEFINE(rx_msgq, 16);
static const struct can_filter telem_filter = {
    .id    = SDV_CAN_BASE_ID,
    .mask  = 0x7F0,
    .flags = 0,
};

static uint32_t latched_hf;
static bool     have_hf;

void sdv_send_cmd(int node, int action)
{
    char buf[24];
    int n = snprintk(buf, sizeof(buf), "C,%d,%d\n", node, action);
    for (int i = 0; i < n; i++) uart_poll_out(link_uart, buf[i]);
    printk("K3,tx,cmd,node=%d,action=%d\n", node, action);
}

static void handle_link_line(const char *line)
{
    if (line[0] != 'L' || line[1] != ',') {
        return;
    }
    char *p = (char *)line + 2;
    unsigned long node = strtoul(p, &p, 10); if (*p++ != ',') return;
    unsigned long sig  = strtoul(p, &p, 10); if (*p++ != ',') return;
    unsigned long sq   = strtoul(p, &p, 10); if (*p++ != ',') return;
    unsigned long val  = strtoul(p, &p, 10);

    printk("K3,rx,node=%lu,sig=%lu,seq=%lu,val=%lu\n", node, sig, sq, val);

    if (node != SDV_NODE_POWERTRAIN) {
        return;
    }
    if (sig == SIG_HEAP_FREE) {
        latched_hf = (uint32_t)val;
        have_hf = true;
    } else if (sig == SIG_LOOP_LATENCY && have_hf) {
        detector_step((uint16_t)sq, latched_hf, (uint32_t)val);
        have_hf = false;
    }
}

int main(void)
{
    printk("K3,boot,ok\n");

    link_uart = DEVICE_DT_GET(DT_NODELABEL(lpuart1));
    bool link_ok = device_is_ready(link_uart);

    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    bool can_ok = false;
    if (device_is_ready(can_dev)) {
        int ret = can_start(can_dev);
        if (ret == 0 || ret == -EALREADY) {
            can_add_rx_filter_msgq(can_dev, &rx_msgq, &telem_filter);
            can_ok = true;
            printk("K3,can,ok\n");
        } else {
            printk("K3,can,start_err=%d\n", ret);
        }
    } else {
        printk("K3,can,unavailable_sim\n");
    }

    if (can_ok) {
        while (1) {
            struct can_frame frame;
            if (k_msgq_get(&rx_msgq, &frame, K_FOREVER) == 0) {
                struct sdv_telem_frame *tf = (struct sdv_telem_frame *)frame.data;
                printk("TELEM,%lld,%u,%u,%u,%u\n",
                       k_uptime_get(), tf->node, tf->signal, tf->seq, tf->value);
            }
        }
    } else if (link_ok) {
        printk("K3,link,ok\n");
        char line[64];
        int idx = 0;
        unsigned char c;
        while (1) {
            while (uart_poll_in(link_uart, &c) == 0) {
                if (c == '\n' || c == '\r') {
                    if (idx > 0) {
                        line[idx] = '\0';
                        handle_link_line(line);
                        idx = 0;
                    }
                } else if (idx < (int)sizeof(line) - 1) {
                    line[idx++] = (char)c;
                }
            }
            k_msleep(5);
        }
    } else {
        printk("K3,err,no_transport\n");
        return -1;
    }
    return 0;
}
