# CLAUDE.md — session context for this repository

## What this repo is

The **Zephyr + Renode simulation slice** of the SDV Fault Prediction &
Self-Healing diploma thesis (Maior Cristian-Alexandru, ULBS, demo December
2026, defense July 2027). Sibling of `folder-structures-0code` (the full
end-state structure PoC). This repo holds only what the simulation combo can
deliver with **no physical board**: K1/K3 Zephyr firmware, the Renode bench,
fault injection, and the dataset factory. All code files are currently
**comment-only** (brief spec of what each file will do); real code lands
following the bring-up order in `README.md`. Coverage analysis and verified
platform facts: `docs/simulation-coverage.md`.

## The knowledge base — use the vault

`vault/` is the Obsidian research brain (mirrored from
`folder-structures-0code`) and the **source of truth for research decisions**.
Consult it before answering design questions or making changes:

- `vault/00-MOCs/HOME.md` — entry point; MOCs index everything by topic.
- `vault/30-Architecture/` — figures + **ADRs (authoritative here)**.
- `vault/10-Literature/`, `20-Concepts/`, `40-Experiments/`, `50-Thesis/`,
  `60-Roadmap/`, `99-Daily/`, `_templates/`.

When work produces a decision or result, offer to update the matching vault
note (use `_templates/`). Note filenames contain em-dashes (—) and spaces —
quote paths in shell commands.

## Hard rules from the architecture (do not violate in any generated code)

- On-vehicle AI may act · cloud AI only proposes · off-vehicle AI only explains.
- The plain-C supervisor holds the final veto; it always preempts inference.
- Only whitelist actions: RESTART / DEGRADED_MODE / LOAD_SHED.
- S32K3 has **no NPU** — int8 + CMSIS-NN on the Cortex-M7 ("S32K3-class
  (S32K388)" in Renode).
- The conversion package is **litert-torch** (ex ai-edge-torch); `model.eval()`
  + output-parity check are mandatory.
- Time-series splits only; never random splits. Scalers fit on train only.
- Only inject faults with analytical ground truth.
- STRETCH items stay out of the tree until their gate passes.

## Platform facts pinned for this repo (verified July 2026)

- Zephyr board target: `mr_canhubk3/s32k344` (only in-tree S32K3 board).
- Renode platform: upstream `platforms/cpus/nxp-s32k388.repl`; console
  lpuart2; CAN on `can0`–`can7` (NXP_FlexCAN).
- No S32K1 platform in Renode master — K1 nodes are S32K3-class day-one;
  dts2repl from Zephyr S32K1 boards is the upgrade path.
- SocketCAN bridge is native in Renode (`CreateSocketCANBridge`), replacing
  the once-planned `renode_vcan_bridge.py`.

## Conventions

- Branch/commit conventions: push to `main` per the owner's instruction.
- Python via `uv` (never conda), Python 3.13.
- Telemetry schema is frozen by ADR-007: (timestamp, signal, value, label,
  true_time_to_failure).
