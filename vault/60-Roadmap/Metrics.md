---
tags: [roadmap]
---
# Metrics

Detection: precision/recall + ROC-AUC per fault class; [[false positive rate]] per hour of normal operation (headline credibility metric).
Prediction: lead-time MAE/RMSE vs the ANALYTICAL true time-to-failure ([[ADR-011 — Fault taxonomy resolved for December]]); GNN propagation top-1/top-2 node accuracy (must beat an MLP on concatenated node features).
Recovery: [[MTTR]]; recovery success rate; count of supervisor vetoes (proof the [[safety cage]] is live).
Cost: detector latency (ms, DWT->CYCCNT) and RAM/flash on [[S32K3]] (tensor-arena size, shrunk empirically); the float32 vs [[int8]] before/after table — size/latency/RAM gained vs AUC lost (AUC drop < 2 points = defensible); bus load overhead of telemetry (%); uplink volume.
Availability: uptime with vs without self-healing across the injection campaign (≥30 incidents; every number traces to a dated W&B run ID + injection seed).
