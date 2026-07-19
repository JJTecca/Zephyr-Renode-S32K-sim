---
tags: [concept]
aliases: [ai-edge-torch]
---
# litert-torch

Google's converter from [[PyTorch]] models to `.tflite` flatbuffers for [[LiteRT for Microcontrollers]].
**Renamed from ai-edge-torch** (Skills Atlas correction, 2026-07): `pip install litert-torch`;
`litert_torch.convert(model.eval(), sample_inputs).export('detector.tflite')`.
Two hard rules from [[ADR-005 — Train in PyTorch, deploy via litert-torch to LiteRT int8]]:
`model.eval()` before converting, and an output-parity check (`np.allclose(pytorch_out, tflite_out)`)
on **every** conversion — a silently-wrong conversion is the nastiest bug in this project.
