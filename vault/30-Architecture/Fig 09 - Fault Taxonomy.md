---
tags: [architecture, figure]
---
# Figure 09 — Fault Taxonomy

## What it shows (one paragraph)
The resolved December set, tiered by defensibility — flagship classes have ANALYTICAL ground truth. [[memory leak]] → OOM ★ (heap slope · PREDICT ⏱ · restart before OOM · 100% flagship) · [[CAN error frames]] → bus-off ★ (TEC/REC 127→256 · PREDICT ⏱ · load-shed · 100%, ⚠ August gate: verify Renode) · timing → deadline-miss (response-time variance · EARLY-WARNING, not RUL · restart/rebalance) · [[acoustic anomaly detection]] ([[MIMII]] FFT bands · DETECTION ONLY — never draw a lead-time · flag) · [[thermal drift]] → threshold ([[C-MAPSS]] synthetic · METHOD DEMO · [[degraded mode]] · **stretch**). Rule: only inject faults with analytical ground truth, so prediction error is exactly measurable.

## Design decisions embodied here
- [[ADR-011 — Fault taxonomy resolved for December]]
- [[ADR-006 — Fault data is generated, reproducibly]]

## Open questions about this figure
-

## Experiments that validate this figure
-

## Changes log
- 2026-07 — initial version (v1 diagrams)
- 2026-07 — **v2 sync to the July system map** (S32N & S32K5 removed; no NPU on S32K3)
