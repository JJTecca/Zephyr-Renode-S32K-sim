---
tags: [literature]
topics: [anomaly-detection]
authors: Malhotra et al.
year: 2016
venue: ICML Workshop
zotero: 
status: done
---
# EXAMPLE — LSTM-based Encoder-Decoder for Multi-sensor Anomaly Detection

*(Delete me once you have real notes — I exist to show the format.)*

## In one sentence (my words)
Train a sequence autoencoder only on healthy telemetry; at runtime, high reconstruction error = anomaly, no labeled faults needed.

## Why it matters for MY thesis
Directly justifies the DETECT design in [[Fig 03 - Real-Time Loop]]: I also lack labeled fault data at scale, so reconstruction-error scoring is the right fit. See [[Denoising Autoencoder]].

## Key claims + evidence
- Works on unlabeled normal data → matches my data-scarcity constraint.

## Links
- Related concepts: [[Reconstruction Error]], [[Denoising Autoencoder]]
- Feeds experiment: [[2026-07-XX AE baseline on C-MAPSS]]
