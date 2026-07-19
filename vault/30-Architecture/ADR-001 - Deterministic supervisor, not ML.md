---
tags: [adr]
status: accepted
date: 2026-07-05
---
# ADR-001 — The safety supervisor is deterministic plain C, not ML

## Context
Self-healing actions (restart, migrate, degrade) can themselves cause harm. Committees and safety engineers distrust stochastic components with actuation authority (ISO 26262 / SOTIF concern, flagged in the thesis-challenges analysis).

## Decision
The final veto gate is deterministic, rule-based plain C. All ML (on-ECU TinyML, central advisory models, off-vehicle LLM) may only propose; only the supervisor may approve an actuation.

## Alternatives considered
- ML-based supervisor — rejected: not certifiable, unbounded worst-case behavior.
- No supervisor (trust the detector) — rejected: single stochastic point of failure with actuation power.

## Consequences
- Good: clean certifiability story; the whole trust argument of the thesis rests here.
- Bad / accepted risk: supervisor rules may veto genuinely good recoveries → measure veto rate in experiments.

## Links
- Affects: [[Fig 03 - Real-Time Loop]], [[Fig 06 - Safety Boundary]]
- Concept: [[Safety Cage Pattern]]
