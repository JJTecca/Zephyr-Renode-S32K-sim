---
tags: [architecture, figure]
---
# Figure 03 — Real-Time Loop

## What it shows (one paragraph)
SENSE → DETECT ([[int8]] [[denoising autoencoder]] on the M7 via [[LiteRT for Microcontrollers]] + [[CMSIS-NN]]) → PREDICT (local trend/[[RUL]] → time-to-failure) → SAFETY SUPERVISOR (deterministic plain-C veto) → ACT ([[service restart]] / [[degraded mode]] / load-shed). The whole [[closed loop]] runs on the [[S32K3]]; bounded latency, sub-second. The [[GNN]] is NOT in this loop — it advises from the cloud (Fig 04). Inference runs at a priority BELOW the supervisor: the supervisor always preempts the AI — that scheduling decision IS the [[safety cage]] in practice.

## Design decisions embodied here
- [[ADR-003 — The supervisor is deterministic plain C]]
- [[ADR-005 — Train in PyTorch, deploy via litert-torch to LiteRT int8]]

## Open questions about this figure
-

## Experiments that validate this figure
-

## Changes log
- 2026-07 — initial version (v1 diagrams)
- 2026-07 — **v2 sync to the July system map** (S32N & S32K5 removed; no NPU on S32K3)
