---
tags: [adr]
status: accepted
---
# ADR-006 — Fault data is generated, reproducibly

**Status:** accepted · 2026-07

**Decision.** Labeled fault telemetry is produced by [[Renode]]-simulated nodes plus [[SocketCAN]]/[[vcan]], with scripted [[fault injection]] per [[Fault taxonomy v1]]; public datasets ([[MIMII]], [[C-MAPSS]]) are used to de-risk models before our data exists. Datasets are versioned with [[DVC]].

**Why.** Real automotive fault data is an industrial secret; a reproducible generation rig is itself a thesis contribution and makes every experiment repeatable.
