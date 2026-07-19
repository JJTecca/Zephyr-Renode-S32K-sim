---
tags: [architecture, figure]
---
# Figure 05 — Data Backbone

## What it shows (one paragraph)
Telemetry up from K3: [[MQTT]] (Mosquitto) → [[Redpanda]] → [[InfluxDB]]/[[TimescaleDB]] + [[pgvector]] → [[FastAPI]]. Consumers: [[GNN]] advisory ★ (proposes), [[Grafana]] + [[human-in-the-loop]] panel (WebSocket live), [[LLM reporter]] (retrieves from pgvector — stretch). Down to K3: proposals + [[OTA update]] model. Hosted on [[Railway]] EU-West (Amsterdam, GDPR) — transport/storage only, never actuates. Topics: vehicle/{id}/ecu/{node}/signal/{name}; QoS 1 incidents, QoS 0 telemetry; measure bus-load overhead (%) per [[Metrics]].

## Design decisions embodied here
- [[ADR-002 — Heavy AI lives in the cloud]]
- [[ADR-004 — Cloud proposes, K3 disposes]]
- [[ADR-008 — Grafana for telemetry, React only for the HITL panel]]

## Open questions about this figure
-

## Experiments that validate this figure
-

## Changes log
- 2026-07 — initial version (v1 diagrams)
- 2026-07 — **v2 sync to the July system map** (S32N & S32K5 removed; no NPU on S32K3)
