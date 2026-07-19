---
tags: [concept]
aliases: [Fault Taxonomy]
---
# Fault taxonomy v1

Historical four-class set ([[thermal drift]], [[timing jitter]], [[CAN error frames]] burst,
[[memory leak]] trend), locked in [[ADR-009 — Fault taxonomy v1 has four classes]] —
**superseded by the resolved December set** in [[ADR-011 — Fault taxonomy resolved for December]]:
memory-leak → OOM ★ · CAN → bus-off ★ · timing → deadline-miss · acoustic (detection-only) ·
thermal demoted to stretch. Rule: only inject faults with analytical ground truth.
