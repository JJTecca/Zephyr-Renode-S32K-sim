---
tags: [architecture, figure]
---
# Figure 06 — Safety Boundary

## What it shows (one paragraph)
Now TWO locations, not three — the old S32N band collapsed into the cloud. On-vehicle · K3: certifiable AI (int8 detector + trend predictor) — MAY ACTUATE; small · quantized · certifiable. ⛔ The red line: deterministic [[safety supervisor]] — plain C, not AI — whitelist restart / degraded-mode / load-shed. Cloud advisory ML ★: [[GNN]] propagation/root-cause — powerful but not certifiable; may recommend, never command; wired to the supervisor, never to an actuator. Cloud reporting: [[LLM reporter]] + dashboards — experimental + human-facing; NEVER ACTUATES. Away from the metal: more powerful, less trusted.

## Design decisions embodied here
- [[ADR-003 — The supervisor is deterministic plain C]]
- [[ADR-004 — Cloud proposes, K3 disposes]]

## Open questions about this figure
-

## Experiments that validate this figure
-

## Changes log
- 2026-07 — initial version (v1 diagrams)
- 2026-07 — **v2 sync to the July system map** (S32N & S32K5 removed; no NPU on S32K3)
