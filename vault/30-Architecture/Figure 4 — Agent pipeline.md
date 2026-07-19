---
tags: [figure]
---
# Figure 4 — Agent pipeline

> **Superseded** — merged into [[Fig 04 - Agent Pipeline]] (2026-07 v2 sync). Kept for link stability.

Cloud-side pipeline: Analyst (anomaly context) → Predictor ([[GNN]]) → Root-cause → **proposal** → down to the on-vehicle Supervisor (rules gate) → Reporter ([[LLM reporter]], audit only). Shared memory: [[InfluxDB]] + [[pgvector]].
