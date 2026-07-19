# Zephyr-Renode-S32K-sim — the simulation-first slice of the SDV thesis

The Zephyr + Renode bench for the SDV Fault Prediction & Self-Healing diploma
project. This repo holds exactly the part of the end-state system that the
simulation combo can deliver **without any physical board**: the K1/K3
firmware, the emulated in-vehicle network, deterministic fault injection, and
the dataset factory. Like its sibling `folder-structures-0code`, every code
file is currently **comment-only** — a brief spec of what the file will do —
so structure and research decisions land before code.

**Coverage:** the bench is ~30% of the end-state tree, directly unblocks
another ~15% (`ml/` trains on its datasets), and ~95% of the thesis needs no
hardware at all — full numbers and verified platform facts in
[`docs/simulation-coverage.md`](docs/simulation-coverage.md).

## The stack (verified upstream, July 2026)

- **Zephyr v4.2** app builds for `mr_canhubk3/s32k344` — the only in-tree
  S32K3 board, and the same MR-CANHUBK344 planned as backup hardware: one ELF
  serves sim and silicon.
- **Renode** runs those unmodified ELFs on the upstream **S32K388** platform
  ("S32K3-class (S32K388)", no NPU — hard rule), multi-node on one CAN hub,
  with a native SocketCAN bridge to host `vcan0`.
- **Fault injection** writes a firmware-exposed control block in *virtual
  time* → same seed, byte-identical logs, analytical ground truth only.

## Map

| Path | What it is |
|---|---|
| `west.yml` | T2 workspace manifest (zephyr + hal_nxp, pinned) |
| `firmware/common/telemetry.h` | Frozen ADR-007 schema + fault-control block |
| `firmware/k1_edge/` | Sense-only node app — never actuates |
| `firmware/k3_hub/` | Loop host: sense_rx → (detect → veto → act next) |
| `sim/renode/` | Platforms, boot topology, sanity demo, fault hooks |
| `sim/configs/` | One YAML per fault class (analytical ground truth only) |
| `sim/run_campaign.py` | One command → labelled, reproducible CSV |
| `scripts/` | vcan bring-up · build all ELFs · launch bench |
| `tests/renode/` | Robot-Framework boot + CAN-path smoke test |
| `.github/workflows/ci.yml` | Build + simulate + reproducibility gate |
| `docs/simulation-coverage.md` | The % analysis + verified platform facts |
| `vault/` | Obsidian research brain (mirrored; source of truth) |

## Bring-up order (when code lands)

1. `renode sim/renode/sanity_shell.resc` — prove the toolchain with zero build.
2. `scripts/build_all.sh` — west workspace + three ELFs.
3. `renode sim/renode/boot_topology.resc` + `start` — 2×K1 + K3 on one bus.
4. `scripts/setup_vcan.sh` + SocketCAN bridge — candump/Wireshark on the host.
5. `sim/run_campaign.py --config sim/configs/memory_leak.yaml --seed 42` —
   first labelled dataset; same seed twice → identical bytes.

---

Everything below is the end-state map, mirrored from
[`folder-structures-0code`](https://github.com/JJTecca/folder-structures-0code)'s
README — the target this repo grows into.

# SDV Fault Prediction & Self-Healing — Structure PoC

Fault prediction + self-healing for a Software-Defined Vehicle. K1 edge → K3 zonal hub → cloud, with a deterministic plain-C supervisor holding the veto. Every file below is **comment-only** (research PoC). Research brain lives in `vault/` (Obsidian, source of truth).

## What each file is responsible for

**`firmware/`** — on-vehicle · C/C++
| File | Responsibility |
|---|---|
| `common.h` | Frozen telemetry schema (timestamp, signal, value, label, true_TTF) + CAN IDs + fault enum |
| `k1_edge.c` | Sense-only nodes (heap slope · TEC/REC · body · acoustic FFT) + CAN TX — **never actuates** |
| `k3_hub.c` | The on-vehicle loop host: sense_rx → predict(RUL) → act → uplink + RTOS priority table |
| `detect_litert.cpp` | int8 autoencoder inference on the M7 (LiteRT for MCU, static arena, CMSIS-NN) |
| `supervisor.c` | **THE VETO** — deterministic C + whitelist (restart / degraded-mode / load-shed) |

**`simulation/`** — the dataset factory
| File | Responsibility |
|---|---|
| `run_campaign.py` | One command: boot 2×K1+1×K3 → inject → labelled CSV (same seed → identical output) |
| `fault_injection.py` | Injector + scheduler + labeler — analytical ground truth only |
| `renode_vcan_bridge.py` | Renode FlexCAN ↔ host SocketCAN (vcan0) |
| `renode/boot_topology.resc` | Boot the whole network on one CAN bus |
| `renode/k1_edge.repl`, `k3_hub_s32k388.repl` | Renode platform definitions |
| `renode/fault_hooks.py` | Register / frame / timed injection in virtual time |
| `configs/*.yaml` | One fault class each (memory_leak, can_bus_off, timing_deadline_miss, acoustic) |

**`ml/`** — Python 3.13 + uv
| File | Responsibility |
|---|---|
| `data.py` | Loaders + rolling windows (slope/variance) + time-series-only splits |
| `detect.py` | The autoencoder + threshold selection + IsolationForest/OC-SVM baselines |
| `predict.py` | Trend/RUL regressor → time-to-failure |
| `gnn.py` | Cross-ECU propagation GNN (2-layer GCN, 5-node graph) + MLP baseline |
| `deploy.py` | int8 quantize + litert-torch convert (.eval()) + parity check + C-array export |
| `eval.py` | Metrics (FPR/h) + lead-time MAE/RMSE + campaign table + W&B logging |
| `notebooks/` | Exploration → public-data baselines → AE → GNN |

**`cloud/`** — Railway EU-West · advisory only
| File | Responsibility |
|---|---|
| `docker-compose.yml` | The whole backbone in one file (laptop → Railway) |
| `api.py` | FastAPI: telemetry queries + GNN proposal endpoint + WebSocket push |
| `advisory_gnn.py` | GNN proposal worker — **proposes only, never actuates** |
| `ingest.py` | MQTT → Redpanda → time-series store |
| `storage.sql` | Timescale hypertables + incident/proposal tables + pgvector |
| `ota.py` | "OTA model down" — versioned, hash-checked detector updates |
| `mosquitto.conf` | MQTT broker config |
| `grafana-dashboard.md` | Fault-timeline dashboard spec (the demo screen) |

**`hitl/`** — React + Recharts · engineer never actuates
| File | Responsibility |
|---|---|
| `App.jsx` | Panel shell + WebSocket client |
| `components.jsx` | FaultTimeline · AnomalyScorePanel · ProposalReview · SupervisorVetoLog |

**Support**
| Path | Responsibility |
|---|---|
| `tests/` | `test_firmware.c` (veto/whitelist) · `test_ml.py` (splits/parity/metrics) · `test_integration.py` (e2e) |
| `scripts/` | `setup_vcan.sh` · `bootstrap_env.sh` (uv) · `run_demo.sh` (the 10-min demo) |
| `datasets/` | `dataset_card.md` — DVC-tracked (git = pointer, DVC = bytes) |
| `docs/` | Flat reference drop-folder: figures, ADRs, safety notes, metrics, roadmap |
| `vault/` | Obsidian research brain — **source of truth** for all research decisions |
| `.github/workflows/ci.yml` | Lint+pytest, firmware build, parity gate, nightly seeded campaign |

## What we expect to detect / predict

The resolved December set. **Flagships have analytical ground truth** — a computable true time-to-failure — so the lead-time claim is exactly measurable. We only inject faults whose ground truth is known.

| Fault class | Signal | Claim | Recovery | Confidence |
|---|---|---|---|---|
| **Memory-leak → OOM** ★ | heap slope | **PREDICT** ⏱ (time-to-failure countdown) | restart before OOM | flagship — the healed class |
| **CAN → bus-off** ★ | TEC/REC (127 → 256) | **PREDICT** ⏱ | load-shed | flagship — ⚠ gated on Renode exposing TEC/REC |
| Timing → deadline-miss | response-time variance | **EARLY-WARNING** (detect, not RUL) | restart / rebalance | high — fallback flagship |
| Acoustic anomaly | mic FFT bands (MIMII) | **DETECT ONLY** (no lead-time) | flag | detection only |
| Thermal → threshold | temp slope (C-MAPSS) | method demo | degraded mode | **stretch — not in tree** |

- **PREDICT ⏱** = we draw a countdown to failure and act *before* it. **EARLY-WARNING** = we flag rising risk but claim no lead-time. **DETECT ONLY** = we flag the anomaly, never a countdown.
- **Cross-ECU prediction (the GNN, headline):** given the graph state at time *t*, predict *which ECU degrades next* (top-1 / top-2) — fault **propagation**, not just single-node detection.

## Hardware needed

- **Baseline (plan of record, €0):** Renode only — S32K388 + S32K118 models + SocketCAN/vcan, on one Linux/WSL2 laptop. Sufficient for the entire thesis; never block on hardware.
- **Cloud:** Railway (EU-West, Amsterdam, GDPR) — free/hobby tier.
- **Backup board (~$179):** MR-CANHUBK344 (S32K344 M7, 4 MB flash, 512 KB SRAM, 6× CAN FD) — one board hosts the whole topology. Order, but develop as if it hasn't arrived.
- **Later (~$129, Jan–Mar 2027):** S32K144EVB-Q100 as a physical K1 node — adds realism, not a new claim.
- **Skip:** S32K344-WB, S32K3X8EVB, S32G GLDBOX. **Note:** S32K3 has **no NPU** — inference is int8 + CMSIS-NN on the Cortex-M7 (in Renode: "S32K3-class (S32K388)").

## Deliverables — by confidence

### 1 · 100% deliverable (built on skills I already have)
Plain C, embedded, and the backend/UI stack I know.
- **Deterministic C supervisor + whitelist** (the safety cage) — `firmware/supervisor.c`, `tests/test_firmware.c`
- **On-vehicle RTOS loop + sense firmware** — `firmware/k3_hub.c`, `k1_edge.c`, `common.h`
- **Cloud backbone** (ingest → store → serve → OTA) — `cloud/api.py`, `ingest.py`, `storage.sql`, `ota.py`, `docker-compose.yml`, `mosquitto.conf`
- **React HITL panel** — `hitl/`
- **Grafana dashboard** (easy win) — `cloud/grafana-dashboard.md`
- **CI** — `.github/workflows/ci.yml`

### 2 · Not sure, but the concept isn't hard (new tools, recipe-driven)
The Python/ML skill gap — well-trodden, learnable in the summer.
- **Telemetry data pipeline** (pandas windows, leakage-safe time-series splits) — `ml/data.py`
- **Anomaly detector** (autoencoder + threshold + baselines + metrics) — `ml/detect.py`, `predict.py`, `eval.py`
- **On-MCU inference** (int8 quantize → litert-torch → LiteRT on the M7) — `ml/deploy.py`, `firmware/detect_litert.cpp`
- **Renode multi-node emulation + fault injection + labelled dataset** — `simulation/`, `datasets/`
- **Data backbone transport** (MQTT / Redpanda / Timescale) — `cloud/ingest.py`
- **SocketCAN/vcan bring-up** — `scripts/setup_vcan.sh`
- **ISO 26262 / SOTIF framing + safety case** (reading, not coding) — `docs/` safety notes
- **DVC dataset versioning** — `datasets/`

### 3 · Risky (novel, or gated on an external unknown)
- **The cross-ECU GNN** — the headline and an open research question; only a contribution if it beats the MLP baseline — `ml/gnn.py`, `cloud/advisory_gnn.py`
- **CAN → bus-off flagship** — gated on the **August check**: does Renode's FlexCAN expose TEC/REC → bus-off? If not, swap to timing/deadline-miss — `simulation/configs/can_bus_off.yaml`
- **Renode ↔ host SocketCAN bridge** — the riskiest integration; test first (July) — `simulation/renode_vcan_bridge.py`
- **The full vertical slice on target silicon** — injected fault → int8 inference on S32K3-class → anomaly score. **September go/no-go** for December.
- **LLM reporter (Qwen RAG)** — stretch, kept out of the tree; first thing to cut.
