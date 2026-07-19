---
tags: [moc]
---
# Safety MOC

Nothing actuates unless the deterministic [[safety supervisor]] (plain C on [[S32K3]]) approves — the
[[safety cage]] pattern. Framing: [[ISO 26262]], [[ASIL]], [[SOTIF]]. Self-healing is defined narrowly
(see [[self-healing]] and [[ADR-009 — Fault taxonomy v1 has four classes]]): [[service restart]],
[[degraded mode]], load shedding — never AI rewriting code.

Key papers: [[Avizienis 2004 — Dependability Taxonomy]], [[Kephart 2003 — Autonomic Computing]] ([[MAPE-K]]),
[[Salay 2017 — ISO 26262 and Machine Learning]], [[Borg 2018 — Safely Entering the Deep]].
Explainability for the audit trail: [[Lundberg 2017 — SHAP]], [[XAI]].
