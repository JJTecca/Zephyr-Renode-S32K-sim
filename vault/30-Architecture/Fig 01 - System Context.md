---
tags: [architecture, figure]
---
# Figure 01 — System Context

## What it shows (one paragraph)
Vehicle ⇄ Railway cloud ⇄ engineer. Vehicle: n × [[S32K1]] sense-only edge nodes + one [[S32K3]] zonal hub (Cortex-M7, on-board AI + the veto) on [[CAN FD]] ([[SocketCAN]]/[[vcan]]), fully [[Renode]]-emulated, unmodified firmware. Uplink [[MQTT]] telemetry; downlink proposals + [[OTA update]] models. Cloud is a *logical role*, not a data-center — physically [[Railway]] EU-West + one laptop; co-location does not collapse the [[trust boundary]]. The engineer ([[human-in-the-loop]] React panel + [[Grafana]]) NEVER actuates. Rule: on-vehicle AI may act · cloud AI only proposes · off-vehicle AI only explains.

## Design decisions embodied here
- [[ADR-001 — Two-tier in-vehicle topology]]
- [[ADR-002 — Heavy AI lives in the cloud]]
- [[ADR-004 — Cloud proposes, K3 disposes]]

## Open questions about this figure
-

## Experiments that validate this figure
-

## Changes log
- 2026-07 — initial version (v1 diagrams)
- 2026-07 — **v2 sync to the July system map** (S32N & S32K5 removed; no NPU on S32K3)
