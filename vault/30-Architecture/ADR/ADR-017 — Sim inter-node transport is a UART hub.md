---
tags: [adr]
status: accepted
date: 2026-08-12
---
# ADR-017 — Sim inter-node transport is a Renode UART hub

## Context
The in-vehicle bus is [[CAN FD]] ([[ADR-007 — CAN FD only for v1]]), and the two-tier
topology ([[ADR-001 — Two-tier in-vehicle topology]]) requires the K1 edges to deliver
telemetry to the K3 zonal hub so the hub can **gather and act** (detect → veto → act)
and uplink to the cloud ([[Fig 05 - Data Backbone]]).

In Renode, the guest [[FlexCAN]] driver will not initialise on our stack: the upstream
S32K388 model gives Zephyr v4.2.0 a **0 Hz CAN clock**, so `can_calc_timing` fails
(`failed to set timing (err -134)`). Antmicro's stripped `mr_canhubk3.repl` (clock tags
+ flip-flops) makes the clock resolve but its tags are tuned to their Zephyr build and
**abort our v4.2.0 in pre-kernel init**. Making CAN work in sim is bespoke Renode
platform engineering that re-breaks on every Zephyr/Renode bump. The thesis must not
block on it — everything runs on Renode, no board on any critical path
([[ADR-012 — Renode is the plan of record]]).

## Decision
We will carry **edge→hub telemetry over a Renode `CreateUARTHub` serial bus** in
simulation, standing in for CAN FD. The same firmware uses CAN on silicon: K1 dual-emits
(link-UART **and** CAN-when-up), K3 reads CAN when ready else the link-UART — one ELF,
runtime fallback, mirroring the existing "UART streams even when CAN is down" pattern.

## Alternatives considered
- **Fix the FlexCAN clock tags for v4.2.0** — rejected *for now*: bespoke, version-fragile
  platform work; deferred to the physical board where CAN is real and the bus-off flagship
  and bus-load metrics actually live.
- **Host-side collector reading each edge UART straight to MQTT** — rejected as the primary
  path: it bypasses the on-vehicle hub's gather/act role (the thing ADR-001 is about).
  Retained only as the *cloud-uplink* bridge downstream of K3.

## Consequences
- Good: the full **edge → hub → cloud** loop, on-vehicle detection, and the
  supervisor-approved heal are demonstrable entirely in Renode, €0, no boards — unblocking
  the September vertical slice, October cloud backbone, and November closed loop.
- Good: the transport swap is a wire change only; no thesis claim about prediction,
  self-healing, or the ML/GNN depends on it.
- Bad / accepted risk: **CAN-FD-specific results are NOT sim-provable** — bus-load %
  overhead, the bus-off flagship, and the *strongest* form of bus-mediated cross-ECU
  propagation are validated on the MR-CANHUBK344 board ([[ADR-012 — Renode is the plan of record]]).
  This substitution MUST be stated in the thesis so no claim rests on the sim wire being CAN.

## Links
- Affects: [[Fig 02 - Hardware Topology]], [[Fig 05 - Data Backbone]]
- Relates: [[ADR-001 — Two-tier in-vehicle topology]], [[ADR-007 — CAN FD only for v1]], [[ADR-012 — Renode is the plan of record]]
