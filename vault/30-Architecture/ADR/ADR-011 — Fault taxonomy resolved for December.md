---
tags: [adr]
status: accepted
---
# ADR-011 — Fault taxonomy resolved for December

**Status:** accepted · 2026-07 · supersedes [[ADR-009 — Fault taxonomy v1 has four classes]]

**Decision.** The December set is re-tiered by how defensible each claim is. Flagship classes must
have **analytical ground truth** — a computable true time-to-failure — so prediction error is
exactly measurable. Never inject a fault whose ground truth is a guess.

| Class | Signal | Claim | Recovery | Confidence |
|---|---|---|---|---|
| [[memory leak]] → OOM ★ | heap slope | PREDICT ⏱ | [[service restart]] before OOM | 100% · flagship (the healed class) |
| [[CAN error frames]] → bus-off ★ | TEC/REC 127→256 | PREDICT ⏱ | load-shed | 100% · ⚠ verify Renode |
| [[timing jitter]] → deadline-miss | response-time variance | EARLY-WARNING (not RUL) | restart / rebalance | High · detection |
| [[acoustic anomaly detection]] | mic FFT bands ([[MIMII]]) | DETECTION ONLY — never a lead-time | flag | 100% · no lead-time |
| [[thermal drift]] → threshold | temp slope ([[C-MAPSS]] synthetic) | METHOD DEMO | [[degraded mode]] | **STRETCH** |

**What changed vs ADR-009.** Acoustic (detection-only) is IN; [[thermal drift]] is demoted to
stretch; the CAN class is sharpened from "error burst" to the analytical TEC/REC → bus-off
countdown; claims are tiered (PREDICT ⏱ vs EARLY-WARNING vs DETECTION ONLY) so no class
overclaims.

**Gate (August).** Verify Renode's FlexCAN actually exposes TEC/REC → bus-off **before**
building the flagship claim on it. If it doesn't, swap that flagship for timing/deadline-miss
immediately — decided at the gate, not in November.
