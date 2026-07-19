---
tags: [adr]
status: accepted
---
# ADR-005 — Train in PyTorch, deploy via litert-torch to LiteRT int8

**Status:** accepted · 2026-07 · **amended 2026-07** (package rename + parity rule)

**Decision.** One training framework ([[PyTorch]]); conversion via [[litert-torch]]
(ex **ai-edge-torch** — renamed upstream) to .tflite; [[int8]] post-training [[quantization]];
on-target inference via [[LiteRT for Microcontrollers]] integrated with [[NXP eIQ]] tooling.
[[ONNX]] is the desktop sanity-check and the format eIQ also ingests — fallback path only.

**Mandatory practices.** `model.eval()` before every conversion; automatic output-parity
check (`np.allclose(pytorch_out, tflite_out)`) asserted in CI on every converted model.

**Why.** Research ergonomics + NXP-friendly deployment without splitting effort across
frameworks. Keep ONE model and convert it — never maintain two.
