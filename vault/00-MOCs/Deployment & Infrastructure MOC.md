---
tags: [moc]
---
# Deployment & Infrastructure MOC
*The path from PyTorch checkpoint to the K3's Cortex-M7 (no NPU — int8 + [[CMSIS-NN]]), and the off-vehicle Railway backbone (Fig 05).*

## Pipeline
PyTorch → [[litert-torch]] (`model.eval()` + parity check) → `.tflite` → quantize [[int8]] (full-integer, calibration set) → eIQ / [[LiteRT for Microcontrollers]] → S32K3 Cortex-M7 with [[CMSIS-NN]] kernels → model as C byte array, const in flash

## Core concepts
- [[LiteRT for Microcontrollers]] · [[NXP eIQ]] · [[litert-torch]] · [[CMSIS-NN]] · [[MQTT]] → [[Redpanda]] ingest · [[OTA update]]

## Key questions
- Latency (DWT->CYCCNT) + tensor-arena/RAM budget on the M7 — measured, not assumed.
- float32 vs int8: size / latency / RAM gained vs AUC lost (before/after table = a result; AUC drop < 2 points = defensible).
- Telemetry rate the MQTT→Redpanda ingest must sustain; bus-load overhead (%).

## Experiments on this topic
```dataview
TABLE status, result FROM "40-Experiments" WHERE contains(topics, "deployment") SORT file.name DESC
```
