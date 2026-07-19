---
tags: [figure]
---
# Figure 5 — Data backbone

> **Superseded** — merged into [[Fig 05 - Data Backbone]] (2026-07 v2 sync). Kept for link stability.

Vehicle edge → [[MQTT]] broker → [[Redpanda]] → [[InfluxDB]]/[[pgvector]] → [[FastAPI]] → [[Grafana]] + React HITL panel, on [[Railway]] EU-West; learning loop: retrain → quantize → [[OTA update]].
