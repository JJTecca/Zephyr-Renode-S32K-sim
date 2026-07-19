---
tags: [concept]
topics: [anomaly-detection]
---
# Denoising Autoencoder

## Definition (my words)
An autoencoder trained to reconstruct clean input from a corrupted version. Trained only on healthy telemetry, it reconstructs normal patterns well and abnormal ones poorly — so [[Reconstruction Error]] becomes an anomaly score.

## Why it matters in this thesis
It is the DETECT model in [[Fig 03 - Real-Time Loop]], quantized to int8 and deployed on the K3's Cortex-M7 with CMSIS-NN — S32K3 has no NPU ([[Deployment & Infrastructure MOC]]). The stronger claim: it fits a plain M7 real-time budget with no accelerator. Needs no labeled fault data, which sidesteps the data-scarcity risk.

## Sources
- [[EXAMPLE - Malhotra 2016 - LSTM Anomaly Detection]]

## Related
- [[Quantization (int8)]] · [[Isolation Forest]] (baseline to beat)
