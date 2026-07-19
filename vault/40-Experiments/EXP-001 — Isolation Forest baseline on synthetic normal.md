---
tags: [experiment]
date: 2026-07-20
mlflow_run: ""
result: pending
---
# EXP-001 — Isolation Forest baseline on synthetic normal

**Hypothesis:** an [[Isolation Forest]] on windowed [[telemetry]] statistics detects injected
[[CAN error frames]] bursts with ROC-AUC > 0.85, giving DETECT a floor to beat.
**Config:** 60 s windows, 12 features/node, contamination=0.02, [[scikit-learn]] defaults.
**Result:** _pending_
**Decision / next:** if AUC < 0.85 revisit features before touching the [[autoencoder]].
