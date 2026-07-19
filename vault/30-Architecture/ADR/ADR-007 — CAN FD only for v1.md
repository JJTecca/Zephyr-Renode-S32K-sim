---
tags: [adr]
status: accepted
---
# ADR-007 — CAN FD only for v1

**Status:** accepted · 2026-07

**Decision.** The only in-vehicle network in v1 is [[CAN FD]]. No Ethernet/TSN, no SOME/IP, no DDS until after December.

**Why.** One bus = one telemetry schema, one fault surface ([[CAN error frames]]), and the K1/K3 boards speak it natively. TSN adds weeks for zero December value.
