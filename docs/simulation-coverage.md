# Simulation coverage — how much of the project the Zephyr + Renode combo delivers

Answer to: *"what is achievable with the simulation will be first integrated
in the repository — but how much % of all of that?"* Measured against the
75-file end-state tree in `folder-structures-0code` (vault excluded).

## The three numbers

| Scope | Share | What it means |
|---|---|---|
| **Sim bench itself** (this repo, now) | **~30%** | `firmware/` + `simulation/` + firmware/integration tests + vcan/demo scripts + CI + datasets — these files *are* the Zephyr+Renode combo and are populated here (22 of 75 files). |
| **Directly unblocked by the sim** | **+15% → ~45%** | All of `ml/` (11 files) trains on the labelled CSVs the bench produces. No boards needed — but nothing to train on until the bench runs. |
| **Achievable with zero hardware, total** | **~95%** | Everything else (`cloud/`, `hitl/`, `docs/`) never touches a board. Per the plan of record, Renode-only is sufficient for the entire thesis. |

The missing ~5% — the only things a physical board (MR-CANHUBK344) adds:

1. **Defensible on-target latency/memory numbers** — Renode's virtual time
   proves logic and ordering, not cycle-accurate ms figures for the
   sub-second-loop claim.
2. **Bus realism** for the demo ("adds realism, not a new claim").

## Per-fault-class coverage in Renode

| Fault class | Simulable today? | Note |
|---|---|---|
| memory_leak → OOM ★ | **YES** | fault-control block + heap stats; analytical ttf = free/rate |
| timing → deadline-miss | **YES** | busy-spin injection, deterministic in virtual time |
| CAN → bus-off ★ | **GATED** | August check: NXP_FlexCAN TEC/REC exposure unverified upstream |
| acoustic (MIMII) | **YES** (replay) | external dataset, detection-only — no ttf exists |

## Verified platform facts (July 2026, upstream sources)

- Renode master ships `platforms/cpus/nxp-s32k388.repl` (quad Cortex-M7,
  8 MB flash, 8× NXP_FlexCAN `can0–7`, 16× LPUART, console `lpuart2`) plus a
  working Zephyr shell demo (`scripts/single-node/nxp-s32k388_zephyr.resc`).
  FlexCAN was validated by Antmicro against Zephyr's CAN counter sample
  (normal + loopback).
- **Correction to the earlier scope note:** there is **no S32K118 (or
  S32K344) platform in Renode master** — the K1 nodes run day-one on the
  S32K3-class platform; the S32K1 path is dts2repl from Zephyr's
  `ucans32k1sic`/`s32k148_evb` devicetrees (S32K116–148 SoCs are upstream in
  Zephyr).
- Zephyr's only in-tree S32K3 board is `mr_canhubk3/s32k344` — same family
  memory map as the S32K388 model, and the same MR-CANHUBK344 board already
  planned as backup hardware, so one ELF serves sim and silicon.
- Renode ≥ 1.15.1 has a native SocketCAN bridge
  (`machine CreateSocketCANBridge "socketcan" "vcan0"`), which **replaces the
  planned `renode_vcan_bridge.py`** — the riskiest integration in the July
  test plan is now an upstream feature.

Sources: [Renode S32K support (Antmicro)](https://antmicro.com/blog/2025/02/renode-support-for-nxp-s32k),
[Zephyr Project article](https://www.zephyrproject.org/testing-nxp-s32k-automotive-general-purpose-mcu-in-renode-simulation-with-zephyr-rtos/),
[nxp-s32k388.repl](https://github.com/renode/renode/blob/master/platforms/cpus/nxp-s32k388.repl),
[MR-CANHUBK3 board docs](https://docs.zephyrproject.org/latest/boards/nxp/mr_canhubk3/doc/index.html),
[Renode CAN host integration](https://renode.readthedocs.io/en/latest/host-integration/can.html).
