---
tags: [adr]
status: accepted
---
# ADR-001 — Two-tier in-vehicle topology

**Status:** accepted · 2026-07

**Decision.** The in-vehicle system has exactly two tiers: 2–3 [[S32K1]] edge nodes (sense-only, no AI, cheap DSP feature extraction) connected over [[CAN FD]] to one [[S32K3]] zonal hub carrying the only on-vehicle AI plus the [[safety supervisor]]. The former S32K5 and S32N tiers are removed.

**Why.** These are the boards actually available; a two-tier star is the smallest topology that still exhibits *cross-ECU* fault propagation (K1→K3 and K1→K1 via shared bus); fewer tiers = a closable loop by December.

**Consequences.** The GNN's graph = K1 nodes + K3 + bus + services (still non-trivial). All heavy AI must move off-vehicle → [[ADR-002 — Heavy AI lives in the cloud]].
