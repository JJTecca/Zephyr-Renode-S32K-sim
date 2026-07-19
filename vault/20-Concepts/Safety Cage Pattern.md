---
tags: [concept]
topics: [safety]
---
# Safety Cage Pattern

## Definition (my words)
An architectural pattern where a simple, deterministic, verifiable component wraps a complex/stochastic one (the ML), holding final authority over any action that affects the physical system. The ML proposes; the cage disposes.

## Why it matters in this thesis
It IS Figure 6. It's the answer to the committee question "how can AI self-healing be safe?" — the supervisor is plain C, not AI, and nothing below the veto line may actuate. See [[ADR-001 - Deterministic supervisor, not ML]].

## Related
- [[ASIL-D]] · [[Fail-Operational vs Fail-Safe]] · [[Fig 06 - Safety Boundary]]
