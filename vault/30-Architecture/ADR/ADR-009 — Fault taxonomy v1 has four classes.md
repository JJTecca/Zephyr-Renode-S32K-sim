---
tags: [adr]
status: superseded
---
# ADR-009 — Fault taxonomy v1 has four classes

**Status:** superseded · 2026-07 → see [[ADR-011 — Fault taxonomy resolved for December]]

**Decision.** V1 predicts and heals exactly four fault classes: [[thermal drift]], [[timing jitter]], [[CAN error frames]] burst, [[memory leak]] trend. Each has a defined injection method, a detectable signature, and at least one whitelisted recovery ([[service restart]], [[degraded mode]], bus load shedding, node reset).

**Why.** "Self-healing of all failures" is unfalsifiable; four injectable, measurable classes are defensible in front of a committee and coverable by December.
