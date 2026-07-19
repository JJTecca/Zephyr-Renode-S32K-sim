---
title: "Inductive Representation Learning on Large Graphs"
authors: "W. Hamilton, R. Ying, J. Leskovec"
year: 2017
venue: "NeurIPS"
arxiv: "1706.02216"
status: unread
score: 6
tags: [paper]
---
# Inductive Representation Learning on Large Graphs

## Summary (my words to verify on first read)
Learns aggregator functions instead of per-node embeddings, so the model generalizes to unseen nodes/graphs.

## Relevance to my thesis
Matters if the topology changes (a node added/removed at runtime — which is exactly a self-healing scenario); keeps the [[GNN]] inductive.
