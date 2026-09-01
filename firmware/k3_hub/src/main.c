/*****************************************************************************
* File:        main.c
* Description: K3 zonal hub -- gathers edge telemetry, runs the on-vehicle
*              anomaly detector (float autoencoder, L2), and prints the score.
* Layer:       firmware/k3_hub  (on-vehicle loop host: gather -> detect)
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include "telemetry.h"
#include "ae_model.h"
#include "actions.h"

static const struct device *can_dev;
static const struct device *link_uart;

CAN_MSGQ_DEFINE(rx_msgq, 16);
static const struct can_filter telem_filter = {
    .id    = SDV_CAN_BASE_ID,   /* 0x100 */
    .mask  = 0x7F0,
    .flags = 0,
};

#define AE_WIN   10          /* dataset.py rolling window */
#define AE_DT_S  0.1f        /* 100 ms tick */
#define AE_W1    (AE_WIN + 1)

static float    hf_hist[AE_W1], ll_hist[AE_W1];
static uint32_t det_tick;
static uint32_t latched_hf;
static bool     have_hf;

static void dense(const float *w, const float *b, const float *in, float *out,
                  int no, int ni, int relu)
/*  step1  z = (x − ae_mu)/ae_sd            = [−136,   −1360,   −0.488,  −0.095]
    step2  h1 = ReLU(W0·z + b0)   (8) = [0, 0, 335.48, 0, 74.84, 0, 180.37, 325.82]
        h2 = ReLU(W1·h1 + b1)  (3) = [385.55, 180.34, 156.00]      ← bottleneck
        h3 = ReLU(W2·h2 + b2)  (8) = [0, 449.84, 0, 0, 233.54, 245.08, 0, 0]
        o  =      W3·h3 + b3   (4) = [−6.51, −10.13, 209.57, 22.28] ← rebuild
    step3  score = mean((o − z)²)          = 470883.74 */
{
    for (int o = 0; o < no; o++) {
        float acc = b[o];
        for (int i = 0; i < ni; i++) {
            acc += w[o * ni + i] * in[i];
        }
        out[o] = (relu && acc < 0.f) ? 0.f : acc;
    }
}

static float ae_score(const float x[AE_D0])
{
    float z[AE_D0], h1[AE_D1], h2[AE_D2], h3[AE_D3], o[AE_D4];
    for (int i = 0; i < AE_D0; i++) {
        z[i] = (x[i] - ae_mu[i]) / ae_sd[i];         /* identical scaling to training */
    }
    dense((const float *)ae_w0, ae_b0, z,  h1, AE_D1, AE_D0, 1);
    dense((const float *)ae_w1, ae_b1, h1, h2, AE_D2, AE_D1, 1);
    dense((const float *)ae_w2, ae_b2, h2, h3, AE_D3, AE_D2, 1);
    dense((const float *)ae_w3, ae_b3, h3, o,  AE_D4, AE_D3, 0);
    float mse = 0.f;
    for (int i = 0; i < AE_D4; i++) {
        float d = o[i] - z[i];
        mse += d * d;
    }
    return mse / AE_D4;                              /* reconstruction error */
}

static void detector_step(uint16_t seq, uint32_t heap_free, uint32_t loop_latency)
{
    float hf = (float)heap_free, ll = (float)loop_latency;
    hf_hist[det_tick % AE_W1] = hf;
    ll_hist[det_tick % AE_W1] = ll;

    uint32_t n   = det_tick < AE_WIN ? det_tick : AE_WIN;
    uint32_t old = (det_tick - n) % AE_W1;
    /* heap_free_slope    = (7976 − 9336) / (10 × 0.1) = −1360      (full leak rate)
       loop_latency_slope = (0 − 0)       / (10 × 0.1) = 0 */
    float hf_slope = n ? (hf - hf_hist[old]) / (n * AE_DT_S) : 0.f;
    float ll_slope = n ? (ll - ll_hist[old]) / (n * AE_DT_S) : 0.f;
    det_tick++;

    float x[AE_D0] = { hf, hf_slope, ll, ll_slope };  /* FEATURES order */
    float s = ae_score(x);
    int alarm = s > AE_THRESHOLD;
    int whole = (int)s;
    int milli = (int)((s - (float)whole) * 1000.f);
    printk("K3,score,seq=%u,score=%d.%03d,alarm=%d\n", seq, whole, milli, alarm);
    if (alarm) {
        printk("K3,observer,notify,src=%u,action=%u\n",
               SDV_NODE_POWERTRAIN, SDV_RESTART);
    }
}

/* Parse one telemetry line from the link UART: "L,node,signal,seq,value".
 * Latch heap_free on sig 1; run the detector when loop_latency (sig 5) closes
 * the tick -- one (heap_free, loop_latency) sample per tick. */
static void handle_link_line(const char *line)
{
    if (line[0] != 'L' || line[1] != ',') {
        return;
    }
    char *p = (char *)line + 2;
    /* Convert a string to an unsigned long integer. */
    unsigned long node = strtoul(p, &p, 10); if (*p++ != ',') return;
    unsigned long sig  = strtoul(p, &p, 10); if (*p++ != ',') return;
    unsigned long sq   = strtoul(p, &p, 10); if (*p++ != ',') return;
    unsigned long val  = strtoul(p, &p, 10);

    printk("K3,rx,node=%lu,sig=%lu,seq=%lu,val=%lu\n", node, sig, sq, val);

    if (node != SDV_NODE_POWERTRAIN) {               /* model trained on K1 powertrain */
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