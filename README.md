# Zephyr-Renode-S32K-sim

Zephyr + Renode simulation slice of the **SDV Fault-Prediction & Self-Healing**
thesis (Maior Cristian-Alexandru, ULBS). Two-tier NXP S32 topology emulated in
Renode with **no physical board**: S32K1 edge nodes (sense + inject) stream
telemetry to an S32K3 zonal hub (gather → detect → predict → act). The hub runs
an on-target autoencoder anomaly detector; fault injection + auto-labelling
produce the project's own telemetry datasets.

For architecture, fault classes and the hardware plan see
[`docs/overview.md`](docs/overview.md); for verified platform facts see
[`docs/simulation-coverage.md`](docs/simulation-coverage.md); for on-hardware
wiring see [`docs/hardware-boards.md`](docs/hardware-boards.md).

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

Python packages for the ML pipeline (into the venv):
`pandas scikit-learn torch` (add `librosa soundfile` for the acoustic/MIMII path).

**Real-silicon path (optional).** The simulation needs *no* board. To also run on
hardware (2× S32K344 — FRDM-A-S32K344 edge + T-box/FRDM-A center), add:

| Tool / part | Notes |
|---|---|
| **S32 Design Studio (S32DS)** | ships `arm-none-eabi-gdb` + the P&E `pegdbserver_console.exe` used by `run_silicon.ps1` |
| **P&E / USB Multilink** | one probe per board (`USBMULTILINK`); each board on its own GDB-server port |
| **PuTTY** | serial consoles (115200 8N1), one per board |
| **2× S32K344 board** | FRDM-A-S32K344 (16 MHz) and/or T-box EVB (40 MHz) |
| **CAN wiring + 120 Ω** | CANH/CANL/GND between nodes, **~60 Ω** across the pair (both terminators), 12 V supply; a multimeter for bring-up |

See [`docs/hardware-boards.md`](docs/hardware-boards.md) for the safe wiring /
pin / transceiver rules and [`boards/nxp/s32k344mini/README.md`](boards/nxp/s32k344mini/README.md)
for the out-of-tree board.

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

### Real-silicon path (optional — 2× S32K344)

Runs the *same* fault→telemetry→detect→predict loop on real boards. One board is
the **edge** (sender, `ROLE_EDGE=1`), one the **center** (receiver, `ROLE_EDGE=0`);
they talk over classic CAN @ 125 kbit/s. One firmware (`firmware/k3_silicon`),
selected at build time by `-DNODE={frdma|tbox}` and `-DROLE_EDGE={0|1}`.

```cmd
:: build both ELFs (2x FRDM-A, 16 MHz): build\frdma_edge + build\frdma_center
powershell -ExecutionPolicy Bypass -File scripts\s32k344-2xfrdm-a.ps1

:: flash both, stream telemetry, inject a fault, run the ML pipeline on silicon logs
powershell -ExecutionPolicy Bypass -File scripts\run_silicon.ps1
```

`run_silicon.ps1` flashes each board headless via `pegdbserver_console.exe`
(`load → monitor reset → detach`, which resumes the core), opens a PuTTY console
per board with logging to `evidence\silicon\`, captures ~30 s of clean baseline,
then arms the fault with `scripts\silicon\inject_leak.gdb` (memory leak) or
`inject_busy.gdb` (deadline-miss). Both use `tbreak main → continue` so the fault
vars are written **after** Zephyr zeroes BSS (writing at the reset vector gets
wiped). It then labels the edge log into `datasets\silicon\` and runs the same
`run_campaign.py → dataset.py → baseline.py → predictor.py` pipeline.

For the T-box (40 MHz crystal) use `scripts\s32k344-tbox_os.ps1` (patches the HAL
clock 16→40 MHz for the build, then restores it); for a single FRDM-A use
`scripts\s32k344-mini_build_os.ps1`; for the S32K144 EVB use
`scripts\s32k144-q100_build_os.ps1`.

> Set `$ComEdge` / `$ComCenter` in `run_silicon.ps1` to your actual ports —
> confirm which board is which from its `2BOARD,boot,role=edge|center` line.
> `firmware\board_check` is a console-only heartbeat (no CAN/heap/float) to prove
> a fresh board port before loading the real firmware.

---

## Repository map

**Renode simulation (no board)**

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
| `datasets/sim/*.csv` | Generated labelled telemetry datasets |

**Real silicon (2× S32K344 — added for on-hardware runs)**

| File / dir | Purpose |
|---|---|
| `firmware/k3_silicon/src/main.c` | One firmware, two roles: edge sends `TELEM,` over CAN + arms `sdv_fault_ctl`; center receives via msgq. Async TX (`K_NO_WAIT`), 64 KB leak heap |
| `firmware/k3_silicon/CMakeLists.txt` | `-DNODE={frdma\|tbox}` + `-DROLE_EDGE={0\|1}` selector |
| `firmware/k3_silicon/{frdma,tbox}.{overlay,conf}` | Per-board DT overlay + Kconfig (flexcan0/PTC21 vs flexcan3/PTB11, UART vs semihost console) |
| `firmware/board_check/` | Console-only heartbeat sanity app — isolates a board port from firmware |
| `boards/nxp/s32k344mini/` | Out-of-tree board: S32K344MINI-EVB / FRDM-A-S32K344 (DTS, pinctrl, defconfig, README) |
| `boards/nxp/s32k144evb/` | Out-of-tree board: S32K144EVB-Q100 |
| `scripts/run_silicon.ps1` | Flash both boards → stream telemetry → inject → ML (silicon equivalent of `run_all.ps1`) |
| `scripts/s32k344-2xfrdm-a.ps1` | Builds the edge + center ELFs (2× FRDM-A, 16 MHz) |
| `scripts/s32k344-tbox_os.ps1` | Builds the T-box ELF (patches HAL clock 16→40 MHz, then restores) |
| `scripts/s32k344-mini_build_os.ps1` | Builds a single FRDM-A ELF |
| `scripts/s32k144-q100_build_os.ps1` | Builds the S32K144 EVB ELF |
| `scripts/silicon/inject_{leak,busy}.gdb` | GDB fault injectors — `tbreak main` then set `sdv_fault_ctl` (leak / busy-spin) |
| `datasets/silicon/*.csv` | Labelled telemetry captured on real silicon |
| `docs/hardware-boards.md` | Board / transceiver / pin + safe-wiring reference |
| `Memory_Regions.md` | SRAM / flash memory-map notes |

**Acoustic (MIMII) fault class**

| File / dir | Purpose |
|---|---|
| `ml/feature_mimii.py` | MIMII valve audio → feature CSVs (`datasets/sim/acoustic_valve_*`) |
| `ml/artifacts_acoustic/` | Acoustic AE outputs (separate from the heap/timing AE) |

**Project-wide**

| File / dir | Purpose |
|---|---|
| `documents/` | Thesis scope (thematic plan, chapter structure) + roadmaps |
| `docs/` | Architecture overview, simulation coverage, code style, hardware boards |
| `evidence/` | Captured plots (heap drain per leak rate) |
| `.github/workflows/` | CI (build + Renode robot), dataset generation, PR-title gate |
| `west.yml` | west manifest — Zephyr v4.2.0 + NXP HAL (allowlisted modules) |
