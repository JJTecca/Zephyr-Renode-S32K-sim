---
tags: [experiment]
topics: [anomaly-detection]
status: planned
result: 
wandb: 
commit: 
---
# 2026-07-XX — Autoencoder baseline on C-MAPSS (public data)

*(Example note showing the format — turn me into your real first experiment.)*

## Hypothesis
If I train a denoising autoencoder on healthy C-MAPSS turbofan cycles, then reconstruction error will separate degraded cycles with AUROC > 0.85, beating an Isolation Forest baseline.

## Setup
- Data: NASA C-MAPSS FD001
- Model: [[Denoising Autoencoder]], window=30, PyTorch
- Baseline: Isolation Forest (scikit-learn)
- Commit / tracker: fill on run

## Result
- 

## Links
- Concepts: [[Denoising Autoencoder]], [[Reconstruction Error]]
- Literature: [[EXAMPLE - Malhotra 2016 - LSTM Anomaly Detection]]
