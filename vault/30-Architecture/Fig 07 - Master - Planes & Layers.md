---
tags: [architecture, figure]
---
# Figure 07 — Master — Planes & Layers

## What it shows (one paragraph)
The platform at a glance: 3 planes · 8 layers (L0–L7) · 2 spines. Plane C·META: L7 knowledge & research workflow (this vault). Plane A·ON-VEHICLE (may actuate): L0 silicon & bus substrate [DEC-SOLID] · L1 sense & fault-injection ★ [DEC-SOLID] · L2 on-vehicle AI ★ [DEC-SOLID] · L3 safety supervisor + ACT ★ — the spine of the thesis [DEC-SOLID]. Plane B·OFF-VEHICLE (advisory): L4 data backbone [DEC-SOLID] · L5 cloud advisory intelligence ★ [GNN v0 solid · causal stretch] · L6 reporting & human-in-loop [Grafana solid · LLM stretch]. Spine 1: safety & certification ([[ISO 26262]]/[[ASIL]] · [[SOTIF]] — framing, not certification). Spine 2: DevOps/MLOps (Git · [[Docker]] · CI · [[DVC]] · W&B/[[MLflow]] · pytest). Trust increases toward the metal · power increases toward the cloud · the veto sits on the boundary.

## Design decisions embodied here
- [[ADR-002 — Heavy AI lives in the cloud]]
- [[ADR-010 — December scope cut]]

## Open questions about this figure
-

## Experiments that validate this figure
-

## Changes log
- 2026-07 — initial version (v1 diagrams)
- 2026-07 — **v2 sync to the July system map** (S32N & S32K5 removed; no NPU on S32K3)
