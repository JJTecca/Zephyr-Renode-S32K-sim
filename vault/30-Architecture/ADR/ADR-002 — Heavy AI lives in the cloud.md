---
tags: [adr]
status: accepted
---
# ADR-002 — Heavy AI lives in the cloud

**Status:** accepted · 2026-07

**Decision.** Everything beyond the quantized detector runs on [[Railway]]: the [[GNN]] propagation predictor, [[root cause analysis]], recovery-policy search, retraining, and the [[LLM reporter]]. The [[S32K3]] runs only the [[int8]] autoencoder and the deterministic supervisor.

**Why.** The K3 has no NPU-class accelerator; industry predictive maintenance is cloud-centric anyway (fleet-scale trend analysis); this cleanly separates the certifiable on-vehicle path from experimental AI — which *is* the thesis's trust-boundary contribution.

**Consequences.** Cloud outputs are advisory → [[ADR-004 — Cloud proposes, K3 disposes]]. The loop must remain safe when the uplink dies (K3-local detect+act still works).
