---
tags: [adr]
status: accepted
---
# ADR-004 — Cloud proposes, K3 disposes

**Status:** accepted · 2026-07

**Decision.** Cloud AI (GNN, root-cause, policy search) sends *proposals* (signed advisory commands with confidence + rationale) down to [[S32K3]]; the [[safety supervisor]] treats them exactly like local ML outputs — checked against the whitelist and vetoable. The dashboard's [[human-in-the-loop]] can endorse a proposal, which still passes the veto.

**Why.** Keeps one single actuation authority; makes the [[trust boundary]] auditable; tolerates uplink loss.
