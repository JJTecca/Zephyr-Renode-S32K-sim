---
tags: [code-log]
date: 2026-07-06
commit: "abc1234"
---
# Example: telemetry schema frozen (merge abc1234)

*(This is what the [[Git merge pipeline — GitHub Action]] writes automatically.)*
Merged the v1 [[telemetry]] frame layout: per-[[S32K1]] frame packs CPU load, two temperatures,
8 FFT band energies for [[acoustic anomaly detection]], [[timing jitter]] stats, heap watermark and
[[CAN error frames]] counters into one 64-byte [[CAN FD]] frame at 10 Hz. Chosen over two 32-byte
frames to halve arbitration overhead. Unblocks [[fault injection]] scripting and EXP-001.

## Follow-ups
- Is 10 Hz enough lead time for [[thermal drift]]? #question
