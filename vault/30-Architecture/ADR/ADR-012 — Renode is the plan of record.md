---
tags: [adr]
status: accepted
---
# ADR-012 — Renode is the plan of record

**Status:** accepted · 2026-07

**Decision.** The whole thesis runs on [[Renode]] — no physical board is on any critical path.
Renode's S32K344 model is not merged, so we use the **S32K388** model and write
“S32K3-class (S32K388)” in the thesis. Never block on hardware.

**Hardware ladder (Skills Atlas).**
- Baseline (plan of record): Renode only — S32K388 + S32K118 models, [[SocketCAN]]/[[vcan]] · €0 · sufficient for the whole thesis.
- Buy as backup: MR-CANHUBK344 (S32K344 M7, 4 MB flash, 512 KB SRAM, 6× CAN FD) ≈ $179 — one board hosts the whole topology; order now, develop as if it hasn't arrived.
- Later (Jan–Mar 2027, after the demo): + S32K144EVB-Q100 as a physical K1 node ≈ $129 — adds realism, not a new claim.
- Skip: S32K344-WB, S32K3X8EVB, S32G GLDBOX — buy no claim the CANHUB board doesn't.

**Also.** Evaluate NXP **eIQ Time Series Studio** (autoML for on-MCU time-series anomaly
detection) — not a replacement for our [[autoencoder]]; a free baseline to beat.

**Consequences.** Determinism of Renode virtual time IS the reproducibility claim
([[ADR-006 — Fault data is generated, reproducibly]]); the Renode↔[[vcan]] bridge is the riskiest
integration and is tested first (July).
