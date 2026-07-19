---
tags: [adr]
status: accepted
---
# ADR-003 — The supervisor is deterministic plain C

**Status:** accepted · 2026-07

**Decision.** The final actuation gate on [[S32K3]] is hand-written, bounded-latency, MC/DC-testable C implementing a whitelist of recovery actions with rate limits, preconditions and rollback — the [[safety cage]] pattern. No ML in this path.

**Why.** [[ISO 26262]]/[[SOTIF]] arguments require a deterministic envelope; committees distrust "self-healing" precisely because they picture AI acting unsupervised. This ADR is the answer.

**Consequences.** Every recovery action must be expressible as a whitelist entry; anything not expressible is out of scope for actuation.
