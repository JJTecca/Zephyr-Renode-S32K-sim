#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <string.h>
#include "telemetry.h"

static const struct device *can_dev;
CAN_MSGQ_DEFINE(rx_msgq, 16);

/* Same filter K3 uses: id 0x100, mask 0x7F0 -> matches 0x100..0x10F. */
static const struct can_filter telem_filter = {
    .id = SDV_CAN_BASE_ID, .mask = 0x7F0, .flags = 0,
};

int main(void)
{
    printk("LOOP,boot,ok\n");

    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    if (!device_is_ready(can_dev)) {
        printk("LOOP,can,unavailable\n");
        return -1;
    }

    /* Loop this controller's own TX back to its own RX + self-ACK -> no 2nd node. */
    can_set_mode(can_dev, CAN_MODE_LOOPBACK);
    can_start(can_dev);
    can_add_rx_filter_msgq(can_dev, &rx_msgq, &telem_filter);
    printk("LOOP,can,ok\n");

    uint16_t seq = 0;
    while (1) {
        struct sdv_telem_frame tf = {
            .node = SDV_NODE_POWERTRAIN, .signal = SIG_HEAP_FREE,
            .seq = seq, .value = 8000u - seq,
        };
        struct can_frame tx = {
            .id = SDV_CAN_BASE_ID | SDV_NODE_POWERTRAIN, /* 0x101 */
            .dlc = sizeof(tf), .flags = 0,
        };
        memcpy(tx.data, &tf, sizeof(tf));
        can_send(can_dev, &tx, K_MSEC(100), NULL, NULL);

        /* --- the "K3 hub" half: receive the same frame back --- */
        struct can_frame rx;
        if (k_msgq_get(&rx_msgq, &rx, K_MSEC(200)) == 0) {
            struct sdv_telem_frame *r = (struct sdv_telem_frame *)rx.data;
            printk("K3,can,rx,id=%03x,node=%u,sig=%u,seq=%u,val=%u\n",
                   rx.id, r->node, r->signal, r->seq, r->value);
        } else {
            printk("K3,can,rx,timeout\n");
        }
        seq++;
        k_msleep(100);
    }
    return 0;
}