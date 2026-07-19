---
tags: [moc]
---
# Anomaly Detection MOC
*The DETECT stage: int8 autoencoder on the K3's Cortex-M7 via CMSIS-NN — no NPU on S32K3 — plus classical baselines.*

## Core concepts
- [[Denoising Autoencoder]] · [[Reconstruction Error]] · [[Isolation Forest]] · [[Quantization (int8)]]

## Key questions
- What telemetry features (CPU load, temp, timing jitter, CAN error frames) carry the most signal?
- What reconstruction-error threshold balances false positives vs. missed faults?
- How much accuracy does int8 quantization cost on the M7 (CMSIS-NN)? The float32-vs-int8 before/after table is a result.

## Literature on this topic
```dataview
LIST FROM "10-Literature" WHERE contains(topics, "anomaly-detection") SORT file.name
```

## Experiments on this topic
```dataview
TABLE status, result FROM "40-Experiments" WHERE contains(topics, "anomaly-detection") SORT file.name DESC
```
