---
tags: [roadmap]
date: 2026-08-12
---
# Phase — In-sim on-vehicle loop over a UART bus (no hardware)

Realises the on-vehicle **gather → detect → predict → heal** loop entirely in [[Renode]],
using a UART hub as the edge→hub transport per [[ADR-017 — Sim inter-node transport is a UART hub]].
No board on the critical path ([[ADR-012 — Renode is the plan of record]]).

## Goal
Inject a fault on a K1 edge → K3 hub receives it over the UART bus → detects → cloud
predicts time-to-failure → **supervisor-approved** action before failure → visible on the
dashboard. One command, reproducible.

## Design — one ELF, runtime fallback
- New dedicated **link-UART** (`lpuart1`) for inter-node telemetry; `lpuart2` stays console/dataset.
- **K1 edge:** each tick, emit `sdv_telem_frame` on the link-UART *and* on CAN-when-up (console line unchanged).
- **K3 hub:** if CAN ready → RX on CAN; else → RX on link-UART, parse, feed the detector.
  A thin `telem_link` shim hides CAN-vs-UART so the loop logic is identical sim and silicon.
- **Loop (K3):** receive → lightweight detect → cloud proposes heavy prediction
  ([[ADR-002 — Heavy AI lives in the cloud]], [[ADR-004 — Cloud proposes, K3 disposes]]) →
  supervisor disposes within the whitelist (RESTART / DEGRADED_MODE / LOAD_SHED) → act + uplink.

## Steps & exit criteria
| # | Step | Exit criterion |
|---|---|---|
| 1 | `CreateUARTHub` in `boot_topology.resc`; connect each node's `lpuart1` | K1-written bytes appear at K3 in the monitor |
| 2 | K1 firmware: dual-emit telemetry on `lpuart1` | K3 link-UART shows K1 frames arriving |
| 3 | K3 firmware: `telem_link` UART-RX + parse | K3 aggregates all edges live ("rx node N …") |
| 4 | On-hub detector (heap slope) + supervisor whitelist action | K3 emits proposal → veto → RESTART; supervisor veto count > 0 |
| 5 | Host bridge: K3 uplink UART → [[MQTT]] → [[TimescaleDB]] → [[Grafana]] | Injected leak visible on dashboard within seconds |
| 6 | 2–3 K1s on the hub → labelled multi-node dataset | Cross-ECU table feeding the [[GNN]] |

## Delivered in sim vs. board-only
- **Sim (this phase):** full edge→hub→cloud loop, on-vehicle detection + supervisor heal,
  the memory-leak **PREDICT** flagship, multi-node data for the GNN, dashboard — the December demo.
- **Board-only (later, ~$179 MR-CANHUBK344):** CAN-FD realism, the bus-off flagship, and the
  strongest bus-mediated cross-ECU propagation claim. Realism, not new claims.

## Roadmap fit
Executes the September vertical slice + October backbone + November closed loop over the
UART-bus transport, hardware-independent.
