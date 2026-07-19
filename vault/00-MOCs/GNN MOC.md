---
tags: [moc]
---
# GNN MOC

The PREDICT stage: a [[GNN]] over the ECU/service dependency graph forecasts cross-ECU [[fault propagation]].
Runs **in the cloud** (see [[ADR-002 — Heavy AI lives in the cloud]]), built with [[PyTorch Geometric]].

Foundations: [[Kipf 2016 — Graph Convolutional Networks]], [[Velickovic 2017 — Graph Attention Networks]],
[[Hamilton 2017 — GraphSAGE]], [[Wu 2019 — Comprehensive Survey on GNNs]].
Applied: [[Chen 2021 — GNN-based Fault Diagnosis Review]].
Root-cause direction: [[root cause analysis]], [[causal inference]], [[Sharma 2020 — DoWhy]].
