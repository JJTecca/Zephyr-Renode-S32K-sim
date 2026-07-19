---
tags: [adr]
status: accepted
---
# ADR-008 — Grafana for telemetry, React only for the HITL panel

**Status:** accepted · 2026-07

**Decision.** Ingest is [[MQTT]] → [[Redpanda]] → [[InfluxDB]] (+ [[pgvector]] for RAG). Engineering telemetry views are [[Grafana]]; custom React+Recharts is reserved for the bespoke [[human-in-the-loop]] approval panel.

**Why.** Don't rebuild Grafana in React; the HITL panel is the only UI that is a contribution.
