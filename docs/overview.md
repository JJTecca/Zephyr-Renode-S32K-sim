# Project overview — Zephyr-Renode-S32K-sim

The **simulation-first slice** of the SDV Fault Prediction & Self-Healing thesis.
It holds exactly the part of the end-state system the Zephyr + Renode combo can
deliver **with no physical board**: K1/K3 firmware, the emulated in-vehicle
network, deterministic fault injection, and the dataset factory. Sibling of
[`folder-structures-0code`](https://github.com/JJTecca/folder-structures-0code).

- **Coverage:** ~30% of the end-state tree, unblocks another ~15% (`ml/` trains
  on its datasets), ~95% of the thesis needs no hardware. Full numbers +
  verified platform facts: [`docs/simulation-coverage.md`](simulation-coverage.md).
- **Research brain:** `vault/` (Obsidian) is the source of truth for decisions.
- **Build the firmware:** see the repo [`README.md`](../README.md).

## The stack (verified upstream, July 2026)

- **Zephyr v4.2** app builds for `mr_canhubk3/s32k344` — the only in-tree S32K3
  board, same MR-CANHUBK344 planned as backup hardware: one ELF, sim and silicon.
- **Renode** runs the unmodified ELFs on the upstream **S32K388** platform
  ("S32K3-class (S32K388)", no NPU — hard rule), multi-node on one CAN hub, with
  a native SocketCAN bridge to host `vcan0`.
- **Fault injection** writes a firmware-exposed control block in *virtual time*
  → same seed, byte-identical logs, analytical ground truth only.

## Repository map

| Path | What it is |
|---|---|
| `west.yml` | T2 workspace manifest (zephyr + hal_nxp, pinned v4.2.0) |
| `firmware/common/telemetry.h` | Frozen ADR-007 schema + fault-control block |
| `firmware/k1_edge/` | Sense-only node app — never actuates |
| `firmware/k3_hub/` | Loop host: sense_rx → (detect → veto → act next) |
| `sim/renode/` | Platforms, boot topology, sanity demo, fault hooks |
| `sim/configs/` | One YAML per fault class (analytical ground truth only) |
| `sim/run_campaign.py` | One command → labelled, reproducible CSV |
| `scripts/` | vcan bring-up · build all ELFs · launch bench |
| `tests/renode/` | Robot-Framework boot + CAN-path smoke test |
| `.github/workflows/ci.yml` | Build + simulate + reproducibility gate |
| `docs/` | Coverage analysis, verified platform facts, this overview |
| `vault/` | Obsidian research brain (mirrored; source of truth) |

## What we expect to detect / predict

Flagships have **analytical ground truth** (a computable true time-to-failure),
so the lead-time claim is exactly measurable. We only inject faults whose ground
truth is known.

| Fault class | Signal | Claim | Recovery | Confidence |
|---|---|---|---|---|
| **Memory-leak → OOM** ★ | heap slope | **PREDICT** ⏱ | restart before OOM | flagship — the healed class |
| **CAN → bus-off** ★ | TEC/REC (127→256) | **PREDICT** ⏱ | load-shed | flagship — gated on Renode TEC/REC |
| Timing → deadline-miss | response-time variance | **EARLY-WARNING** | restart / rebalance | high — fallback flagship |
| Acoustic anomaly | mic FFT bands (MIMII) | **DETECT ONLY** | flag | detection only |
| Thermal → threshold | temp slope (C-MAPSS) | method demo | degraded mode | stretch — not in tree |

**Cross-ECU prediction (the GNN, headline):** given the graph state at time *t*,
predict *which ECU degrades next* — fault **propagation**, not single-node detection.

## Hardware

- **Baseline (plan of record, €0):** Renode only — S32K388 + SocketCAN/vcan on one
  Linux/WSL2/Windows laptop. Sufficient for the entire thesis; never block on hardware.
- **Cloud:** Railway (EU-West, Amsterdam, GDPR) — free/hobby tier.
- **Backup board (~$179):** MR-CANHUBK344 (S32K344 M7, 4 MB flash, 6× CAN FD) — one
  board hosts the whole topology. Order, but develop as if it hasn't arrived.
- **Later (~$129):** S32K144EVB-Q100 as a physical K1 node — realism, not a new claim.
- **Note:** S32K3 has **no NPU** — inference is int8 + CMSIS-NN on the Cortex-M7.

## Deliverables — by confidence

**1 · 100% deliverable** (plain C, embedded, backend/UI): deterministic C
supervisor + whitelist, on-vehicle RTOS loop + sense firmware, cloud backbone
(ingest → store → serve → OTA), React HITL panel, Grafana dashboard, CI.

**2 · Concept isn't hard** (new tools, recipe-driven): telemetry data pipeline
(leakage-safe time-series splits), anomaly detector (AE + threshold + baselines),
on-MCU int8 inference (litert-torch → LiteRT on the M7), Renode multi-node
emulation + fault injection + labelled dataset, MQTT/Redpanda/Timescale transport,
SocketCAN/vcan bring-up, ISO 26262 / SOTIF framing, DVC dataset versioning.

**3 · Risky** (novel or gated): the cross-ECU GNN (headline, must beat MLP
baseline), CAN → bus-off flagship (gated on the August Renode TEC/REC check),
Renode ↔ host SocketCAN bridge, the full vertical slice on target silicon
(September go/no-go), LLM reporter (stretch — first to cut).

---

*The full end-state map lives in the sibling repo's README:*
[`folder-structures-0code`](https://github.com/JJTecca/folder-structures-0code).
