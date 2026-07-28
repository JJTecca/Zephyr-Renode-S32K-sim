# Zephyr-Renode-S32K-sim — build & run guide (Windows)

The Zephyr + Renode bench for the SDV Fault Prediction & Self-Healing thesis.
This README is a **step-by-step tutorial for building the firmware and running
it in Renode on Windows**, using `cmd.exe`. For what the project *is* — the
architecture, coverage, fault classes, hardware plan — see
[`docs/overview.md`](docs/overview.md).

> Target board: `mr_canhubk3/s32k344` (Cortex-M7). Zephyr **v4.2.0**.
> Renode platform: upstream S32K388 ("S32K3-class").

---

## 0 · Prerequisites (install once)

| Tool | How | Notes |
|---|---|---|
| **Python 3.13** | python.org | used for the venv + `west` |
| **CMake** | winget install Kitware.CMake | on `PATH` |
| **Ninja** | `pip install ninja` (into the venv, step 1) | build backend |
| **Device Tree Compiler** | comes with MSYS2 (`dtc`) or Zephyr deps | `dtc --version` |
| **7-Zip** | https://www.7-zip.org | needed to extract the SDK |
| **Renode** | https://renode.io (1.16.x) | the simulator |
| **git** | git-scm.com | `git config --global core.longpaths true` |

Everything below keeps the workspace and SDK **on `D:`** so the `C:` drive
doesn't fill up. Adjust paths if you use a different drive.

---

## 1 · Workspace + west

Clone into a workspace folder, create a venv, install `west`:

```cmd
mkdir D:\zephyr-ws
cd /d D:\zephyr-ws
git clone https://github.com/JJTecca/Zephyr-Renode-S32K-sim.git

python -m venv .venv
call .venv\Scripts\activate.bat
pip install west ninja
```

Result: the repo sits at `D:\zephyr-ws\Zephyr-Renode-S32K-sim` and the workspace
top dir is `D:\zephyr-ws`.

---

## 2 · Fetch Zephyr + modules

The repo **is** the west manifest repo, so init points west at it, and `update`
pulls Zephyr v4.2.0 + the NXP HAL (allowlisted modules only):

```cmd
cd /d D:\zephyr-ws\Zephyr-Renode-S32K-sim
west init -l .
west update
west zephyr-export
```

- `west init -l .` registers this repo's `west.yml` as the manifest.
- `west update` clones `zephyr\` and `modules\hal\{nxp,cmsis,cmsis_6}\` under
  `D:\zephyr-ws\`.
- `west zephyr-export` registers this Zephyr with CMake so `find_package(Zephyr)`
  resolves — **no `ZEPHYR_BASE` env var needed**.

Verify one Zephyr checkout at v4.2.0:

```cmd
west list
```

You should see `zephyr  zephyr  v4.2.0` and the three `modules\hal\...` entries.

---

## 3 · Install the Zephyr SDK — version must match Zephyr

> ⚠️ **This is the step that bites.** Zephyr **v4.2.0 needs SDK 0.17.x**. The
> newer **SDK 1.0.x is incompatible** (major-version bump) and CMake will reject
> it with *"Could not find a configuration file for package Zephyr-sdk compatible
> with requested version 0.16"*. `west sdk install` reads the required version
> from `zephyr\SDK_VERSION`, so run it **after** step 2 with this workspace active.

Add 7-Zip to `PATH` for this session, then install **only the ARM toolchain**,
onto `D:`:

```cmd
set PATH=%PATH%;C:\Program Files\7-Zip
west sdk install --install-base D:\ -t arm-zephyr-eabi
```

That creates `D:\zephyr-sdk-0.17.x` (~800 MB) and registers it with CMake.
Find the exact folder name:

```cmd
dir D:\ /b | findstr zephyr-sdk
```

Point the toolchain env var at it (substitute the real `0.17.x`) — permanent
via `setx`, and live in this session via `set`:

```cmd
setx ZEPHYR_SDK_INSTALL_DIR "D:\zephyr-sdk-0.17.2"
set ZEPHYR_SDK_INSTALL_DIR=D:\zephyr-sdk-0.17.2
```

<details>
<summary>Manual fallback if <code>--install-base</code> is rejected</summary>

```cmd
type D:\zephyr-ws\zephyr\SDK_VERSION
cd /d D:\
curl -L -o sdk.7z https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.2/zephyr-sdk-0.17.2_windows-x86_64_minimal.7z
"C:\Program Files\7-Zip\7z.exe" x sdk.7z
cd /d D:\zephyr-sdk-0.17.2
setup.cmd
```
At the `setup.cmd` prompts answer: `Y` (GNU), `N` (all targets), `Y` **only for
`arm-zephyr-eabi`** and `N` for every other target, `N` (LLVM), `N` (host tools),
`Y` (register CMake package). Substitute whatever version `SDK_VERSION` printed.
</details>

---

## 4 · Build the firmware

```cmd
cd /d D:\zephyr-ws\Zephyr-Renode-S32K-sim
west build -b mr_canhubk3/s32k344 firmware\k1_edge -d build\k1_powertrain
```

Success ends with a memory-usage report and produces:

```
build\k1_powertrain\zephyr\zephyr.elf
```

Build the K3 hub the same way, and a second K1 instance via its node-ID option:

```cmd
west build -b mr_canhubk3/s32k344 firmware\k3_hub  -d build\k3_hub
west build -b mr_canhubk3/s32k344 firmware\k1_edge -d build\k1_chassis -- -DCONFIG_SDV_NODE_ID=2
```

**Rebuild clean** (after touching `prj.conf`, `Kconfig`, or the board):

```cmd
rmdir /s /q build\k1_powertrain
west build -b mr_canhubk3/s32k344 firmware\k1_edge -d build\k1_powertrain
```

---

## 5 · Run in Renode

**Toolchain sanity (zero build) —** proves your Renode + S32K388 model work.
In the Renode monitor:

```
include @scripts/single-node/nxp-s32k388_zephyr.resc
start
```
A UART window shows a Zephyr shell prompt (`uart:~$`). The wall of
`Unhandled write to ...` warnings is normal — the S32K388 model doesn't
implement every clock/flash peripheral.

**Boot your own ELF —** in the Renode monitor (use forward slashes in paths):

```
mach create "k1_powertrain"
machine LoadPlatformDescription @D:/zephyr-ws/Zephyr-Renode-S32K-sim/sim/renode/k1_edge.repl
sysbus LoadELF @D:/zephyr-ws/Zephyr-Renode-S32K-sim/build/k1_powertrain/zephyr/zephyr.elf
showAnalyzer sysbus.lpuart2
start
```
The UART window prints the boot lines (`K1,boot,...`) and telemetry.

---

## Troubleshooting

| Symptom | Cause → fix |
|---|---|
| `Could not find ... Zephyr-sdk compatible with requested version 0.16` | SDK too new (1.0.x). Install **0.17.x** and repoint `ZEPHYR_SDK_INSTALL_DIR` (step 3). |
| `zephyr/dt-bindings/pinctrl/nxp-s32-pinctrl.h: No such file` | Zephyr ↔ hal_nxp version mismatch (a second, older Zephyr in the tree). Keep **one** Zephyr checkout; `west update` + `west zephyr-export`. |
| `include could not find requested file: zephyr_default` then the compiler test links with `-lkernel32` | `find_package(Zephyr)` didn't run — Zephyr not registered. Run `west zephyr-export`. |
| `west: unknown command "build"` | Wrong dir or manifest not resolving. `cd` into the repo; check `west config manifest.path`. |
| `ninja: not found` | `pip install ninja` inside the active venv. |
| Everything installs onto `C:` | Set `--install-base D:\` for the SDK; redirect caches with `set PIP_CACHE_DIR=D:\caches\pip`. |

---

## What's next (bring-up order)

1. `sanity_shell.resc` — Renode proof, zero build ✅ (step 5).
2. Build all three ELFs (step 4).
3. `sim/renode/boot_topology.resc` + `start` — 2×K1 + K3 on one CAN bus.
4. `scripts/setup_vcan.sh` + SocketCAN bridge — candump/Wireshark on the host.
5. `sim/run_campaign.py --config sim/configs/memory_leak.yaml --seed 42` — first
   labelled dataset; same seed twice → identical bytes.

Architecture, coverage %, fault-class table, hardware plan, deliverables:
[`docs/overview.md`](docs/overview.md) · verified platform facts:
[`docs/simulation-coverage.md`](docs/simulation-coverage.md) · research brain: `vault/`.
