---
title: "TensorFlow Lite Micro: Embedded Machine Learning on TinyML Systems"
authors: "R. David et al."
year: 2020
venue: "MLSys 2021"
arxiv: "2010.08678"
status: unread
score: 8
tags: [paper]
---
# TensorFlow Lite Micro: Embedded Machine Learning on TinyML Systems

## Summary (my words to verify on first read)
Describes the interpreter-based MCU inference runtime (now LiteRT for Microcontrollers): no OS dependence, no dynamic allocation, kernels swappable per target.

## Relevance to my thesis
The runtime that executes our detector on the [[S32K3]]; its memory-arena model dictates our RAM budget math. Anchors [[LiteRT for Microcontrollers]].
