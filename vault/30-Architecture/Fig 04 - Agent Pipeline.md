---
tags: [architecture, figure]
---
# Figure 04 — Agent Pipeline

## What it shows (one paragraph)
A–E straddle the [[trust boundary]]. On-vehicle (K3, may actuate): A·Analyst (telemetry → anomaly score, int8 AE), B·Predictor (anomalies → time-to-failure), D·Supervisor (approves/vetoes → safe command, plain C — THE VETO). Cloud (off-vehicle): C·Root-cause ★ ([[GNN]] propagation — PROPOSES ONLY; its proposal crosses the veto, D decides, never C) and E·Reporter ([[LLM reporter]], Qwen [[RAG]] — NEVER ACTUATES; **stretch**). Shared memory: [[InfluxDB]]/[[TimescaleDB]] + [[pgvector]].

## Design decisions embodied here
- [[ADR-003 — The supervisor is deterministic plain C]]
- [[ADR-004 — Cloud proposes, K3 disposes]]

## Open questions about this figure
-

## Experiments that validate this figure
-

## Changes log
- 2026-07 — initial version (v1 diagrams)
- 2026-07 — **v2 sync to the July system map** (S32N & S32K5 removed; no NPU on S32K3)
