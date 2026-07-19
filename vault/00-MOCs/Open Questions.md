---
tags: [moc]
---
# Open Questions
*One line per unresolved question. Move to ✅ with a link to the note that answered it.*

## Open
- [ ] What exact fault taxonomy for v1? → feeds [[Simulation & Fault Injection MOC]]
- [ ] Autoencoder input window size vs. the K3 tensor-arena/RAM budget (Cortex-M7, no NPU)?
- [ ] AUGUST GATE — does Renode's FlexCAN expose TEC/REC → bus-off? If not, swap that flagship for timing/deadline-miss (see [[ADR-011 — Fault taxonomy resolved for December]]).
- [ ] Graph representation: ECU-level nodes or service-level nodes?

## Answered
- [x] *(example)* Train in PyTorch or TensorFlow? → PyTorch, convert via [[litert-torch]] (ex ai-edge-torch). See [[ADR-001 - Deterministic supervisor, not ML]] pattern for how to record such decisions.
