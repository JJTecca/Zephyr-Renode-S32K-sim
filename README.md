# Zephyr-Renode-S32K-sim

Zephyr + Renode simulation slice of the **SDV Fault-Prediction & Self-Healing**
thesis (Maior Cristian-Alexandru, ULBS). Two-tier NXP S32 topology emulated in
Renode with **no physical board**: S32K1 edge nodes (sense + inject) stream
telemetry to an S32K3 zonal hub (gather → detect → predict → act). The hub runs
an on-target autoencoder anomaly detector; fault injection + auto-labelling
produce the project's own telemetry datasets.

For architecture, fault classes and the hardware plan see
[`docs/overview.md`](docs/overview.md); for verified platform facts see
[`docs/simulation-coverage.md`](docs/simulation-coverage.md).

> Board `mr_canhubk3/s32k344` (Cortex-M7) · Zephyr **v4.2.0** · SDK **0.17.x** ·
> Renode **1.16.x** · Renode platform: upstream S32K388.

---

## Prerequisites

Install once. Everything lives on `D:` so `C:` doesn't fill up — adjust the drive
if you use another.

| Tool | Install | Notes |
|---|---|---|
| **Python 3.13** | python.org | venv + `west` + the ML pipeline |
| **CMake** | `winget install Kitware.CMake` | on `PATH` |
| **Ninja** | `pip install ninja` (into the venv) | build backend |
| **Device Tree Compiler** | MSYS2 (`dtc`) | `dtc --version` |
| **7-Zip** | 7-zip.org | extracts the Zephyr SDK |
| **Zephyr SDK 0.17.x** | `west sdk install` (step below) | must match Zephyr v4.2.0 — **not** 1.0.x |
| **Renode 1.16.x** | renode.io | the simulator |
| **git** | git-scm.com | `git config --global core.longpaths true` |

Python packages for the ML pipeline (into the venv): `pandas scikit-learn torch`.

---

## Setup + run (project side)

One block, from an empty `D:\zephyr-ws` to a full build → simulate → dataset → ML
run. `west sdk install` reads the required SDK version from `zephyr\SDK_VERSION`,
so it must come **after** `west update`.

```cmd
:: 1 - workspace + west
mkdir D:\zephyr-ws && cd /d D:\zephyr-ws
git clone https://github.com/JJTecca/Zephyr-Renode-S32K-sim.git
python -m venv .venv
call .venv\Scripts\activate.bat
pip install west ninja pandas scikit-learn torch

:: 2 - fetch Zephyr v4.2.0 + NXP HAL (this repo IS the west manifest)
cd /d D:\zephyr-ws\Zephyr-Renode-S32K-sim
west init -l .
west update
west zephyr-export

:: 3 - Zephyr SDK 0.17.x (ARM toolchain only), onto D:
set PATH=%PATH%;C:\Program Files\7-Zip
west sdk install --install-base D:\ -t arm-zephyr-eabi
setx ZEPHYR_SDK_INSTALL_DIR "D:\zephyr-sdk-0.17.2"
set ZEPHYR_SDK_INSTALL_DIR=D:\zephyr-sdk-0.17.2

:: 4 - build + simulate + generate datasets + run ML (one entry point)
powershell -ExecutionPolicy Bypass -File scripts\run_all.ps1
```

`scripts\run_all.ps1` orchestrates the whole loop: builds the K3 hub + 4×K1 ELFs
(`s32k1k3_build_os.ps1`), opens Renode on the topology, injects a fault, parses
the captured UART log into a labelled CSV (`run_campaign.py`), then runs the ML
pipeline (`dataset.py` → `baseline.py` → `predictor.py` → `train_ae.py` →
`quantize.py` → `export_model.py`). Launch it from the **repo root**.

For headless, multi-seed/rate dataset generation (no GUI) use
`scripts\run_all_ci.ps1`; the same flow runs in CI via
[`.github/workflows/dataset.yml`](.github/workflows/dataset.yml).

**Renode-only sanity** (zero build): in the Renode monitor,
`include @sim/renode/sanity_shell.resc` then `start` — a UART window shows the
Zephyr shell (`uart:~$`). The `Unhandled write to ...` warnings are expected
(the S32K388 model omits some clock/flash peripherals).

---

## Repository map

| File / dir | Purpose |
|---|---|
| `firmware/k1_edge/src/main.c` | K1 edge node — samples heap/timing, emits `TELEM,` + `L,` link frames (dual-emit link-UART / CAN-when-up) |
| `firmware/k3_hub/src/main.c` | K3 zonal hub — receives K1 telemetry, runs the float AE detector, emits `K3,score,…alarm=` and `K3,observer,notify` |
| `firmware/common/telemetry.h` | Frozen telemetry schema (ADR-007) + signal IDs |
| `firmware/common/actions.h` | Whitelist action enum (RESTART / DEGRADED_MODE / LOAD_SHED) + node IDs |
| `firmware/common/ae_model.h` | **Generated** AE weights / scaler / threshold — do not edit (from `export_model.py`) |
| `firmware/k1_edge/Kconfig` | `SDV_NODE_ID` build option (node identity) |
| `firmware/*/prj.conf`, `*/app.overlay` | Per-node Zephyr / board config |
| `firmware/osberver/` | Host-only GoF Observer demo — teaching artifact, **not** built into firmware |
| `ml/dataset.py` | CSV load, long→wide pivot, rolling-slope features, time-series split + scaler |
| `ml/train_ae.py` | Trains the denoising autoencoder on normal rows → `ae.pt` + manifest + calib set |
| `ml/quantize.py` | int8 post-training quantization + ROC-AUC parity gate → `ae_int8.npz` |
| `ml/export_model.py` | Emits `firmware/common/ae_model.h` from `ae.pt` |
| `ml/baseline.py` | Detection baselines (ROC-AUC, false-positives/hour) |
| `ml/predictor.py` | Heap-slope → time-to-OOM regression (analytic vs linreg) |
| `ml/gnn.py` | Cross-ECU GNN stub (Sprint 5) |
| `ml/api.py`, `ingest.py`, `metrics.py`, `reporter.py` | Off-vehicle backbone stubs (Sprint 4–5) |
| `ml/artifacts/` | Trained outputs: `ae.pt`, `ae_int8.npz`, `ae_manifest.json`, `calib.npy` |
| `sim/run_campaign.py` | Labels a K1 UART log into a dataset CSV (analytical ttf ground truth) |
| `sim/configs/*.yaml` | Per fault-class parameters (rate, onset, run length) |
| `sim/renode/boot_topology.resc` | Boots the 2×K1 + K3 topology |
| `sim/renode/sanity_shell.resc` | Zero-build Renode shell sanity check |
| `sim/renode/*.repl` | Platform descriptions (`k1_edge`, `k3_hub_s32k388`; `mr_canhubk3` parked) |
| `sim/renode/fault_hooks.py` | Renode monitor commands: `inject_memory_leak` / `inject_busy_spin` / `clear_faults` |
| `scripts/run_all.ps1` | One-shot local: build → Renode → campaign → ML |
| `scripts/run_all_ci.ps1` | Headless multi-seed/rate campaign driver |
| `scripts/s32k1k3_build_os.ps1` | Builds the K3 hub + 4×K1 ELFs |
| `scripts/renode_open.py` | Launches the Renode GUI (Windows) |
| `tests/renode/*.robot` | Renode Robot-Framework tests (see `tests/renode/README`) |
| `datasets/*.csv` | Generated labelled telemetry datasets |
| `documents/` | Thesis scope (thematic plan, chapter structure) + roadmaps |
| `docs/` | Architecture overview, simulation coverage, code style |
| `evidence/` | Captured plots (heap drain per leak rate) |
| `.github/workflows/` | CI (build + Renode robot), dataset generation, PR-title gate |
| `west.yml` | west manifest — Zephyr v4.2.0 + NXP HAL (allowlisted modules) |
