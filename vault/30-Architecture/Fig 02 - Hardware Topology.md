---
tags: [architecture, figure]
---
# Figure 02 — Hardware Topology

## What it shows (one paragraph)
Four [[S32K1]] edge nodes — powertrain (CPU/timing/heap), chassis ([[CAN error frames]]), body (signals), acoustic (mic → FFT bands) — on one [[CAN FD]] bus into the [[S32K3]] zonal hub: Cortex-M7, **no NPU** (int8 + [[CMSIS-NN]]; the detector fits a plain M7 budget with no accelerator), hosting int8 detector · trend/RUL · supervisor veto · ACT. MQTT uplink to the cloud gateway (advisory only, never on the CAN bus). Fully [[Renode]]-emulated — use the S32K388 model and write “S32K3-class (S32K388)” ([[ADR-012 — Renode is the plan of record]]). No S32N, no S32K5.

## Design decisions embodied here
- [[ADR-001 — Two-tier in-vehicle topology]]
- [[ADR-012 — Renode is the plan of record]]

## Open questions about this figure
-

## Experiments that validate this figure
-

## Changes log
- 2026-07 — initial version (v1 diagrams)
- 2026-07 — **v2 sync to the July system map** (S32N & S32K5 removed; no NPU on S32K3)
