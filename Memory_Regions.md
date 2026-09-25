# Memory Regions — S32K3-T-BOX & FRDM-A-S32K344

**Both target boards carry the same MCU: the NXP `S32K344` (lockstep Cortex-M7).**
The **S32K3-T-BOX** (telematics-box reference design, SCH-50735) and the
**FRDM-A-S32K344** (the ex-`S32K344MINI-EVB`, UG10389 FRDM Automotive bundle) differ
in *board integration* — debugger, PMIC, oscillator, connectors, peripherals — **not
in the silicon memory map**. So the two maps below are deliberately identical: same
die, same map. What actually differs between our two *firmware roles* (K1 edge vs K3
hub) is **occupancy**, and what differs between the two *boards* is everything
**outside** the MCU memory (§ "What differs per board").

> Source of truth: NXP S32K3xx data sheet + S32K3 Memories Guide (AN13388), cross-checked
> against Zephyr v4.2.0 devicetree for `mr_canhubk3/s32k344` (same SoC) and the repo's
> out-of-tree `boards/nxp/s32k344mini` port. Addresses/sizes below are what the linker
> actually sees.

---

## Shared silicon memory map — `S32K344` (both boards)

| Region | Address range | Size | Notes |
|---|---|---|---|
| ITCM | `0x0000_0000 – 0x0000_FFFF` | 64 KB | Instruction TCM (0-wait-state code). Cortex-M7 core-local. |
| Program flash (P-Flash) | `0x0040_0000 – 0x007F_FFFF` | 4 MB physical | Code + constants (XIP). Zephyr's `flash0` maps **4048 KB** (`0x3F4000`): a 256 B IVT header + the code partition; the top of P-Flash is reserved (HSE firmware / config — exact size depends on the HSE variant flashed). |
| Data flash (D-Flash) | `0x1000_0000 – 0x1001_FFFF` | 128 KB | EEPROM-style non-volatile data. **Unused** by our firmware. |
| UTEST | `0x1B00_0000 – 0x1B00_1FFF` | 8 KB | One-time-programmable / config block. **Unused.** |
| DTCM | `0x2000_0000 – 0x2001_FFFF` | 128 KB | Data TCM (0-wait-state data). Core-local. |
| SRAM (SRAM_0/1/2, ECC) | `0x2040_0000 – 0x2044_FFFF` | 320 KB | System RAM: `.data`, `.bss`, stacks, heaps. First 32 KB (`0x2040_0000`) is standby-capable (`SRAM0_STDBY`). |
| QuadSPI XIP | `0x6800_0000 –` | (ext.) | External serial NOR (FRDM-A has an on-board MX25L64; T-BOX per its BoM). **Unused** by our firmware today. |

**On-chip RAM total = 512 KB = 320 KB SRAM + 64 KB ITCM + 128 KB DTCM.** (The "512 KB
RAM" you see in marketing *includes* the two TCMs; system SRAM alone is 320 KB.)

---

## Occupancy — K1 edge role (`firmware/k1_edge`, sense + inject)

| Region | Used by K1 | Why |
|---|---|---|
| ITCM | — | Code is XIP from P-Flash. |
| P-Flash | sense loop, telemetry emit, CAN + link-UART drivers, fault hooks | Plain C, no ML runtime → small. |
| DTCM | — | Nothing placed there by default. |
| SRAM | `.data`/`.bss`, thread stacks, system heap (`CONFIG_HEAP_MEM_POOL_SIZE=16 KB`), and the injected-leak arena `leak_heap` (`k_heap`, `LEAK_HEAP_SZ=8 KB`) | The 8 KB arena is what the memory-leak fault drains — bounded, so a leak never corrupts the rest of RAM. |

**Measured footprint (CI, built `-b mr_canhubk3/s32k344`):**

| Linker region | Used | Region size | %age |
|---|---|---|---|
| IVT_HEADER | 256 B | 256 B | 100.00% |
| FLASH | ~61.6 KB | ~4048 KB | ~1.5% |
| RAM (SRAM) | ~34.0 KB | 320 KB | ~10.4% |
| ITCM | 0 B | 64 KB | 0% |
| DTCM | 0 B | 128 KB | 0% |
| IDT_LIST | 0 B | 32 KB | 0% (build-time only) |

---

## Occupancy — K3 hub role (`firmware/k3_hub`, gather → detect → veto → act)

| Region | Used by K3 | Why |
|---|---|---|
| ITCM | — (candidate) | Future home for the int8 inference kernel (fast, 0-wait-state). |
| P-Flash | link-UART RX + parser, the autoencoder detector, `ae_model.h` weight tables (`static const float` → `.rodata`), **plus** the C++ runtime (`CONFIG_CPP`) and TFLite-Micro (`CONFIG_TENSORFLOW_LITE_MICRO`) | These extra libraries are why K3 uses **more flash** than K1. |
| DTCM | — (candidate) | Future home for AE activation buffers / weights for fast inference. |
| SRAM | `.data`/`.bss`, stacks, heap, detector rolling-window buffers (`hf_hist`/`ll_hist`, `AE_W1`), AE working state | Comparable to K1; the ML state is small (float AE, 4→8→3→8→4). |

**Measured footprint:** not separately captured in the current CI log (the printed
report is a K1 image). K3 is expected to use **more flash** (C++ runtime + TFLite-Micro
+ weight tables); SRAM stays comparable. Get exact numbers with
`arm-zephyr-eabi-size -A build/k3_hub/zephyr/zephyr.elf`.

---

## What differs *per board* (none of it changes the memory map)

| Item | S32K3-T-BOX | FRDM-A-S32K344 (ex-S32K344MINI-EVB) |
|---|---|---|
| MCU | S32K344, Cortex-M7 lockstep | S32K344, Cortex-M7 lockstep |
| Silicon memory map | **identical** (table above) | **identical** (table above) |
| On-board debugger | **None** — external SWD/JTAG (PEmicro / J-Link) on the `JTAG Cortex 20` header | PEmicro **OpenSDA** (USB, VCOM) — *not* CMSIS-DAP, so `west flash -r pyocd/linkserver` won't see it; use S32DS or `-r jlink` |
| Power / safety | FS26 + FS56/PF5020 PMIC, backup-battery charger, 12 V rail | FS26 safety PMIC (put in **debug mode** during bring-up or its watchdog resets the MCU) |
| Heavy peripherals | 5G modem, Wi-Fi/BT, eMMC, SD, SJA1110 Ethernet switch, audio, multi-CAN FD/LIN | RGB LED, SSD1306 OLED, buttons, 6× CAN FD PHYs, Ethernet PHY, QSPI NOR |
| Zephyr board port | **none yet** — must port (clone `s32k344mini`, fix XTAL/pinmux/console) | **exists** in-repo: `boards/nxp/s32k344mini` (cloned from `mr_canhubk3`, same SoC) |
| Effort to run our ELF | higher (port + external probe + 12 V bring-up) | low (port already builds; verify console UART / FXOSC / CAN pins on real HW) |

Because the map is board-independent, the **linker configuration and the `.elf` are the
same** across `mr_canhubk3/s32k344` (sim), `s32k344mini` (FRDM-A), and a future T-BOX
port — only the *board* layer (clocks, pinmux, console UART, debugger) changes.

---

## Notes

- **XIP:** code executes in place from P-Flash; only `.data` is copied flash → SRAM at
  boot. TCMs (ITCM/DTCM) are declared by the SoC but nothing is placed there by
  default → 0 bytes used on both roles.
- **IVT_HEADER (256 B @ `0x0040_0000`):** the Image Vector Table the BootROM reads at
  reset to find the app entry point; always 100 % (one header).
- **IDT_LIST (32 KB):** build-time scratch for interrupt-table generation; not in the
  final image.
- **Silicon vs. emulation:** Renode runs the S32K344 ELF on the upstream `s32k388` CPU
  model (a superset — 8 MB flash, 3×256 KB SRAM, same base addresses), which is why the
  smaller K344 image boots unmodified in sim. Emulated per-core TCM sizes differ but are
  moot because the TCMs are unused.
- **Inspect it yourself:** `west build` prints the region table on every link;
  `build/<app>/zephyr/zephyr.map` shows per-symbol placement (`ae_w0` in
  `.rodata`/FLASH, `leak_heap` in SRAM); `linker.cmd` holds the generated addresses
  from the devicetree.

---

## Sources

- NXP — [S32K3-T-BOX reference design (S32K344 MCU)](https://www.nxp.com/design/design-center/development-boards-and-designs/S32K3-T-BOX) · schematic SCH-50735 (attached; pages "S32K344 POWER/PTx", "CAN FD", "5G MODULE")
- NXP — [FRDM-A-S32K344](https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-A-S32K344) · UG10389 "FRDM Automotive Bundle User Guide" (attached; "FRDM-A-S32K344, previously released as S32K344MINI-EVB")
- NXP — S32K3xx Data Sheet & S32K3 Memories Guide (AN13388) for the S32K344 memory map
- Zephyr v4.2.0 devicetree for `mr_canhubk3/s32k344` (same SoC) and repo `boards/nxp/s32k344mini`
