---
tags: [adr]
status: accepted
---
# ADR-010 — December scope cut

**Status:** accepted · 2026-07

**Decision.** By December we ship: the two-tier rig, the generated dataset, the on-K3 detector, the closed loop for **one** fault class end-to-end with **one** recovery action, the cloud backbone with Grafana, and a **v0 cloud GNN in advisory mode**. Cut to future work: RL recovery policies, [[federated learning]], full [[causal inference]], TSN, formal certification.

**Why.** See [[Roadmap July–December]]; the honest cut protects the headline (trust-boundary architecture + reproducible fault-data rig + advisory GNN).
