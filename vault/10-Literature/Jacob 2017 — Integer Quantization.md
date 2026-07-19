---
title: "Quantization and Training of Neural Networks for Efficient Integer-Arithmetic-Only Inference"
authors: "B. Jacob et al."
year: 2017
venue: "CVPR 2018"
arxiv: "1712.05877"
status: unread
score: 7
tags: [paper]
---
# Quantization and Training of Neural Networks for Efficient Integer-Arithmetic-Only Inference

## Summary (my words to verify on first read)
The int8 quantization scheme (scale+zero-point, quantization-aware training) that underlies TFLite/LiteRT integer inference.

## Relevance to my thesis
Explains exactly what happens to our detector during [[quantization]] and why post-training int8 may need QAT if accuracy drops. Anchors [[int8]].
