# CLAUDE.md — session context (kept lean; loads every turn)

## RULE 0 — deliver code in chat, don't push it (owner lifts per-request, in chat)
Default: output complete files in chat with their target path; the owner builds/runs them. No
commits/PRs of source unless the owner authorises *that specific push* in chat (common this
session). Docs/vault pushes are fine when the owner names them; this CLAUDE.md may be edited when
the owner asks.
Authorship + git hygiene (HARD): Claude is NEVER the author, committer, or co-author — every
commit is authored AND committed by the owner (`CristiM <maiorcristian123@gmail.com>`; set
`git config user.name/user.email` to this before committing). Claude never creates branches (local
or remote) and never opens PRs. The ONLY push Claude makes is a force-push straight to `main`, and
ONLY when the owner asks for it in that request — no feature branches, no side branches, no
branch-satisfying pushes for stop-hooks.

## RULE 1 — before an owner-asked PR, satisfy every .github/workflows/*.yml
Re-read the yml; don't guess. Gates: pr-title-check — title `^\[(FEATURE|BUG|HOTFIX|DOCS|REFACTOR|
TEST|CHORE)\]\s.+`, ≤40 chars. ci.yml — builds firmware + Renode robot test. Owner-authorised
pushes to `main` bypass CI.

## RULE 2 — the roadmaps are mandated reading (MUST)
Before any roadmap / plan / "what's next" / scope / feasibility work, READ **both tiers** — do not
invent a parallel plan:
- **FINAL authoritative scope (what MUST ship):** `documents/Plan_tematic_licenta_Maior.docx` — the
  official ULBS diploma thematic plan (deliverables + graphics; deadline **20 Jun 2027**) — and
  `documents/Cuprins_SDV.docx` — the frozen thesis chapter structure (Chaps I–VIII). Every task
  must trace to a deliverable in these two. The thematic plan's 4 fault classes, 2 flagship
  predictions, GNN, C-supervisor, data backbone + dashboard, and ≥30-incident metrics campaign are
  the contract.
- **NEAR-TERM execution cadence:** `documents/SDV_Sim_Roadmap_September.pdf` — the compressed
  simulation-only sprint plan (mid-Aug → 30 Sep 2026; L0–L7; Sprints 0–6 with exit criteria). This
  is *how* we drive toward the final scope; follow its sprints and exit criteria.
- Where they conflict, the official thematic plan wins on *scope*, the Sept plan wins on *ordering*.
  Known conflict: thematic plan lists CAN bus-off as a flagship prediction, but bus-off is NOT
  simulable in Renode (decorative regs, ADR-012) → graduated to the MR-CANHUBK344 board; the sim's
  second flagship is timing/deadline-miss. Flag this honestly, never silently drop a deliverable.
Regenerate any PDF only when the owner asks. `documents/SDV_Dataset_Evolution.pdf` = the raw-UART →
detector trace explainer (teaching aid).

## Context economy (owner request — don't waste the window)
Reuse this file + prior findings; don't re-derive established facts each turn. Renode source is
already at `/workspace/renode/renode` — don't re-clone. Read vault notes only when a decision
needs them. Biggest per-turn cost is the GitHub MCP instruction block (re-injects every turn) —
if PR/issue tools aren't needed, the owner can disconnect that MCP server.

## What this repo is
Zephyr + Renode sim slice of the SDV Fault-Prediction & Self-Healing thesis (Maior
Cristian-Alexandru, ULBS; demo Dec 2026). No board. K1 edge (sense-only) + K3 hub
(gather→detect→veto→act) firmware, Renode bench, fault injection, dataset factory. `vault/` =
Obsidian brain, source of truth (30-Architecture/ = authoritative ADRs; filenames use em-dashes +
spaces → quote in shell). Offer a vault note when work yields a decision (`_templates/`).

## Established this session — do NOT re-investigate
- Firmware is real code now (K1/K3 `main.c`, schema, fault hooks) — not comment-only.
- CAN does NOT init in sim: S32K388 model gives Zephyr v4.2.0 a 0 Hz CAN clock → bit-timing
  `err -134`. `mr_canhubk3.repl` clock-tag platform aborts our v4.2.0 pre-kernel. So repls use
  `using "platforms/cpus/nxp-s32k388.repl"` (boots + UART); `mr_canhubk3.repl` parked in-tree.
- Sim inter-node transport = Renode UART hub on `lpuart1`, standing in for CAN FD (ADR-017). K1
  dual-emits (link-UART + CAN-when-up); K3 RX = CAN if ready else link-UART; one ELF; lpuart2 =
  console. Proven: K3 receives K1 telemetry, no board.
- FlexCAN Renode type: stable 1.16.x = `CAN.S32K3XX_FlexCAN`; master renamed → `NXP_FlexCAN`
  (2026-07-03).
- Workflow: build `scripts/s32k1k3_build_os.ps1` (harden to stop on `$LASTEXITCODE`); run
  `scripts/renode_open.py` → `i @sim/renode/boot_topology.resc`; inject `i fault_hooks.py;
  inject_memory_leak k1_powertrain 128`. CAN-realism items (bus-off, bus-load) → MR-CANHUBK344
  board (ADR-012).
- Next: dataset v1 (`sim/run_campaign.py`) → baseline ML → predict+heal in K3 `handle_link_line`.

## Hard architecture rules (never violate in generated code)
On-vehicle AI may act · cloud only proposes · off-vehicle only explains. Plain-C supervisor holds
the final veto (preempts inference). Whitelist actions only: RESTART / DEGRADED_MODE / LOAD_SHED.
S32K3 has no NPU (int8 + CMSIS-NN on M7). Conversion = litert-torch + `model.eval()` parity check.
Time-series splits only (never random); scalers fit on train only. Only inject faults with
analytical ground truth. Telemetry schema frozen (ADR-007): timestamp, signal, value, label,
true_time_to_failure.

## Platform / conventions
Board `mr_canhubk3/s32k344`, Zephyr v4.2.0. Renode platform `nxp-s32k388.repl`; console lpuart2,
link bus lpuart1. Python via `uv`, 3.13. Push to `main` per owner instruction.
