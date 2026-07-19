---
tags: [architecture]
---
# Architecture v2 — master (rewritten from scratch)

Custom three-tier architecture; no standard template applies, so the description below **is** the reference.
Hardware truth: [[ADR-001 — Two-tier in-vehicle topology]] (no K5, no S32N).

## Tier 0 — Edge sensing: 2–3 × [[S32K1]] (no AI)
Each K1 owns a physical corner of the "vehicle" and produces a fixed-rate telemetry frame on [[CAN FD]]:
CPU load, on-die + external temperature (our temperature signal), microphone/IMU **features** (RMS +
8 FFT band energies computed by cheap DSP — our noise signal, per [[acoustic anomaly detection]]),
task [[timing jitter]] stats, heap watermark (for [[memory leak]] trends) and [[CAN error frames]] counters.
K1 intelligence is deliberately zero-ML: feature extraction, a [[watchdog]], and the ability to *execute*
recovery commands it receives ([[service restart]] of a local task, [[degraded mode]], rate change).

## Tier 1 — Zonal hub: 1 × [[S32K3]] (the only on-vehicle AI)
The K3 fuses all K1 frames plus its own self-telemetry into sliding windows and runs the entire
on-vehicle [[closed loop]]:
1. **SENSE** — collect + timestamp + sanity-check frames.
2. **DETECT** — [[int8]] [[denoising autoencoder]] via [[LiteRT for Microcontrollers]]; [[reconstruction error]] → per-node anomaly score.
3. **PREDICT (local, minimal)** — trend extrapolation only (slope of temperature/heap) for time-to-threshold.
4. **SUPERVISE** — the deterministic [[safety supervisor]] ([[ADR-003 — The supervisor is deterministic plain C]]) checks any proposed action against a whitelist + rate limits + preconditions.
5. **ACT** — approved commands go to the target K1 (or K3 itself); result verified; everything logged.
The K3 also owns the cloud uplink ([[MQTT]]) and applies [[OTA update]]s of the detector after CRC+signature checks. If the uplink dies, tier 0+1 keep working — that is a design invariant.

## Tier 2 — Cloud: [[Railway]] (all the heavy AI, never actuates directly)
Per [[ADR-002 — Heavy AI lives in the cloud]]: [[MQTT]] broker → [[Redpanda]] → [[InfluxDB]] (telemetry)
+ [[pgvector]] (incident/RAG memory) → [[FastAPI]] backend →
[[Grafana]] engineering dashboards + React [[human-in-the-loop]] panel. Cloud AI jobs:
- **PREDICT (global)** — the [[GNN]] over the dependency graph (nodes: K1s, K3, bus, services; edges: comms + resource coupling) forecasting [[fault propagation]] paths and horizons.
- **[[root cause analysis]]** — graph + ([[causal inference]], stretch).
- **Policy suggestion** — ranks whitelist recoveries by predicted MTTR (never invents new actions).
- **[[LLM reporter]]** — [[RAG]]-grounded incident reports and audit records.
- **Learning loop** — retrain → [[quantization]] → signed [[OTA update]] back to K3.
All cloud outputs travel down as *proposals* per [[ADR-004 — Cloud proposes, K3 disposes]].

## The trust boundary (the showpiece)
Green (may actuate): K3 detector + supervisor. Purple (proposes only): cloud GNN/root-cause/policy.
Blue (never actuates): LLM reporter, dashboards, retraining. One red line: the supervisor's veto.
See [[trust boundary]] and [[Figure 6 — Safety and trust boundary]].

## What we predict & self-heal (evidence-based)
The resolved December set per [[ADR-011 — Fault taxonomy resolved for December]]
(flagships have **analytical ground truth**; only such faults are injected):
| Fault class | Signature | Claim | Recovery (whitelisted) |
|---|---|---|---|
| [[memory leak]] → OOM ★ | heap watermark slope ↑ | PREDICT ⏱ | scheduled [[service restart]] before OOM |
| [[CAN error frames]] → bus-off ★ | TEC/REC 127→256 | PREDICT ⏱ (⚠ verify Renode, August gate) | bus load shedding |
| [[timing jitter]] → deadline-miss | deadline-miss variance ↑ | EARLY-WARNING | [[service restart]], priority rebalance |
| [[acoustic anomaly detection]] | mic FFT bands ([[MIMII]]) | DETECTION ONLY | flag |
| [[thermal drift]] → threshold | slow temp slope ↑ vs load | METHOD DEMO · **stretch** | [[degraded mode]] (shed load) |
