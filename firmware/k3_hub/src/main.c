/*****************************************************************************
* File:        main.c
* Description: K3 zonal hub -- gathers edge telemetry and prints it. Receives
*              over CAN when the bus is up (silicon); otherwise over the
*              inter-node link UART (lpuart1) -- the Renode UART hub standing in
*              for CAN FD in simulation (ADR-017).
* Layer:       firmware/k3_hub  (on-vehicle loop host)
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include "telemetry.h"

static const struct device *can_dev;
static const struct device *link_uart;

CAN_MSGQ_DEFINE(rx_msgq, 16);
static const struct can_filter telem_filter = {
    .id    = SDV_CAN_BASE_ID,   /* 0x100 */
    .mask  = 0x7F0,
    .flags = 0,
};

/* Parse one telemetry line from the link UART: "L,node,signal,seq,value".
 * This is the hook point for the next phase (detector + supervisor). */
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
    /* TODO (next phase): feed heap-slope detector; on prediction, supervisor
     * disposes within the whitelist (RESTART / DEGRADED_MODE / LOAD_SHED). */
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
        /* Silicon path: block on CAN RX. */
        while (1) {
            struct can_frame frame;
            if (k_msgq_get(&rx_msgq, &frame, K_FOREVER) == 0) {
                struct sdv_telem_frame *tf = (struct sdv_telem_frame *)frame.data;
                printk("TELEM,%lld,%u,%u,%u,%u\n",
                       k_uptime_get(), tf->node, tf->signal, tf->seq, tf->value);
            }
        }
    } else if (link_ok) {
        /* Sim path: poll the link UART for telemetry lines (ADR-017). */
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