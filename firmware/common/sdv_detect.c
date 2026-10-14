/*****************************************************************************
* File:        sdv_detect.c
* Description: Shared on-vehicle anomaly detector (float autoencoder) and
*              deterministic supervisor, linked into k3_hub and k3_silicon.
* Layer:       firmware/common
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/

#include <zephyr/kernel.h>
#include "telemetry.h"
#include "actions.h"
#include "ae_model.h"
#include "config.h"
#include "sdv_detect.h"

static float    hf_hist[AE_W1], ll_hist[AE_W1];
static uint32_t det_tick, guard_blocks;
static bool     acting;

static int action_allowed(int a)
{
	return a == SDV_RESTART || a == SDV_DEGRADED_MODE || a == SDV_LOAD_SHED;
}

static void dense(const float *w, const float *b, const float *in, float *out, int no, int ni, int relu)
{
	for (int o = 0; o < no; o++) {
		float acc = b[o];
		for (int i = 0; i < ni; i++) acc += w[o * ni + i] * in[i];
		out[o] = (relu && acc < 0.f) ? 0.f : acc;
	}
}

float ae_score(const float *x)
{
	float z[AE_D0], h1[AE_D1], h2[AE_D2], h3[AE_D3], o[AE_D4];
	for (int i = 0; i < AE_D0; i++) z[i] = (x[i] - ae_mu[i]) / ae_sd[i];
	dense((const float *)ae_w0, ae_b0, z,  h1, AE_D1, AE_D0, 1);
	dense((const float *)ae_w1, ae_b1, h1, h2, AE_D2, AE_D1, 1);
	dense((const float *)ae_w2, ae_b2, h2, h3, AE_D3, AE_D2, 1);
	dense((const float *)ae_w3, ae_b3, h3, o,  AE_D4, AE_D3, 0);
	float mse = 0.f;
	for (int i = 0; i < AE_D4; i++) { float d = o[i] - z[i]; mse += d * d; }
	return mse / AE_D4;
}

int supervisor_step(int src, int alarm, float ttf, int proposed)
{
	if (!alarm) return SDV_NONE;
	if (!action_allowed(proposed)) {
		guard_blocks++;
		printk("K3,supervisor,block,src=%d,action=%d,reason=whitelist\n", src, proposed);
		printk("K3,supervisor,guard_blocks=%u\n", guard_blocks);
		return SDV_NONE;
	}
	if (proposed == SDV_RESTART && ttf > TTF_GUARD_S) {
		guard_blocks++;
		printk("K3,supervisor,block,src=%d,action=RESTART,reason=ttf%.1f\n", src, (double)ttf);
		printk("K3,supervisor,guard_blocks=%u\n", guard_blocks);
		return SDV_NONE;
	}
	if (acting) return SDV_NONE;
	acting = true;
	printk("K3,supervisor,approve,src=%d,action=%d,ttf=%.1f\n", src, proposed, (double)ttf);
	return proposed;
}

void detector_step(uint16_t seq, uint32_t heap_free, uint32_t loop_latency)
{
	float hf = (float)heap_free, ll = (float)loop_latency;
	hf_hist[det_tick % AE_W1] = hf;
	ll_hist[det_tick % AE_W1] = ll;
	uint32_t n   = det_tick < AE_WIN ? det_tick : AE_WIN;
	uint32_t old = (det_tick - n) % AE_W1;
	float hf_slope = n ? (hf - hf_hist[old]) / (n * AE_DT_S) : 0.f;
	float ll_slope = n ? (ll - ll_hist[old]) / (n * AE_DT_S) : 0.f;
	det_tick++;

	float x[AE_D0] = { hf, hf_slope, ll, ll_slope };
	uint32_t c0 = k_cycle_get_32();
	float s = ae_score(x);
	uint32_t infer_ns = (uint32_t)k_cyc_to_ns_floor64(k_cycle_get_32() - c0);

	int alarm = s > AE_THRESHOLD;
	int whole = (int)s, milli = (int)((s - (float)whole) * 1000.f);
	printk("K3,score,seq=%u,score=%d.%03d,alarm=%d,infer_ns=%u\n", seq, whole, milli, alarm, infer_ns);

	if (alarm) {
		float drain = -hf_slope; if (drain < 1e-6f) drain = 1e-6f;
		float ttf = heap_free / drain;
		int act = supervisor_step(SDV_NODE_POWERTRAIN, alarm, ttf, SDV_RESTART);
		if (act != SDV_NONE) sdv_send_cmd(SDV_NODE_POWERTRAIN, act);
	} else if (acting) {
		acting = false;
		printk("K3,supervisor,rearm\n");
	}
}
