# Memory Regions — K1 edge & K3 hub

Both firmware images are built for the **same board**, `mr_canhubk3/s32k344`
(Cortex-M7), so the **silicon memory map is identical** for K1 and K3. What
differs is *occupancy* — what each node places into each region — which is why
this file carries a per-node table plus each node's measured build footprint.

> Addresses/sizes: NXP S32K344 reference map. The image is linked for `s32k344`
> and runs on Renode's `s32k388` model (a superset at the same base addresses),
> which is why a K344 image boots unmodified in simulation.

---

## K1 — edge node (`firmware/k1_edge`, sense + inject)

| Region | Address range | Size | Role (K1) |
|---|---|---|---|
| ITCM | `0x0000_0000 – 0x0000_FFFF` | 64 KB | Instruction tightly-coupled RAM (0-wait-state code). **Unused** — code is XIP from flash. |
| Program flash | `0x0040_0000 – 0x007F_FFFF` | 4 MB | Code + constants (XIP): sense loop, telemetry emit, CAN + link-UART drivers, fault-control hooks. Top 176 KB reserved for HSE firmware. |
| Data flash | `0x1000_0000 – 0x1001_FFFF` | 128 KB | EEPROM-style non-volatile data. **Unused.** |
| UTEST | `0x1B00_0000 – 0x1B00_1FFF` | 8 KB | One-time programmable / config block. **Unused.** |
| DTCM | `0x2000_0000 – 0x2001_FFFF` | 128 KB | Data tightly-coupled RAM (0-wait-state data). **Unused.** |
| SRAM (SRAM_0/1/2) | `0x2040_0000 – 0x2044_FFFF` | 320 KB | System RAM — `.data`, `.bss`, thread stacks, system heap (`CONFIG_HEAP_MEM_POOL_SIZE=16 KB`), and the injected-leak arena `leak_heap` (`K_HEAP_DEFINE`, 8 KB). This is the RAM the memory-leak fault drains. |
| QuadSPI XIP | `0x6800_0000 –` | (ext.) | External serial flash. **Unused.** |

**Measured build footprint (CI, `mr_canhubk3/s32k344`):**

| Linker region | Used | Region size | %age |
|---|---|---|---|
| IVT_HEADER | 256 B | 256 B | 100.00% |
| FLASH | ~61.6 KB | ~3.95 MB | 1.49% |
| RAM | ~34.0 KB | 320 KB | 10.37% |
| ITCM | 0 B | 64 KB | 0.00% |
| DTCM | 0 B | 128 KB | 0.00% |
| IDT_LIST | 0 B | 32 KB | 0.00% (build-time only) |

---

## K3 — zonal hub (`firmware/k3_hub`, gather → detect → act)

| Region | Address range | Size | Role (K3) |
|---|---|---|---|
| ITCM | `0x0000_0000 – 0x0000_FFFF` | 64 KB | Instruction tightly-coupled RAM (0-wait-state code). **Unused today** — candidate home for the int8 inference kernel later. |
| Program flash | `0x0040_0000 – 0x007F_FFFF` | 4 MB | Code + constants (XIP): link-UART RX + parser, the autoencoder detector, and the generated `ae_model.h` weight tables (`static const float` → `.rodata`). Also carries the C++ runtime (`CONFIG_CPP`) and TFLite-Micro (`CONFIG_TENSORFLOW_LITE_MICRO`). Top 176 KB reserved for HSE firmware. |
| Data flash | `0x1000_0000 – 0x1001_FFFF` | 128 KB | EEPROM-style non-volatile data. **Unused.** |
| UTEST | `0x1B00_0000 – 0x1B00_1FFF` | 8 KB | One-time programmable / config block. **Unused.** |
| DTCM | `0x2000_0000 – 0x2001_FFFF` | 128 KB | Data tightly-coupled RAM (0-wait-state data). **Unused today** — candidate home for the AE activation buffers / weights for fast inference. |
| SRAM (SRAM_0/1/2) | `0x2040_0000 – 0x2044_FFFF` | 320 KB | System RAM — `.data`, `.bss`, thread stacks, system heap, detector rolling-window buffers (`hf_hist`/`ll_hist`) and AE working state. |
| QuadSPI XIP | `0x6800_0000 –` | (ext.) | External serial flash. **Unused.** |

**Measured build footprint:** not separately captured in the current CI log
(the printed report is for a K1 image). K3 is expected to use **more flash**
than K1 because it links the C++ runtime, TFLite-Micro, and the `ae_model.h`
weight tables; SRAM use stays comparable. Run
`arm-zephyr-eabi-size -A build/k3_hub/zephyr/zephyr.elf` for exact numbers.

---

## Notes

- **XIP:** code executes in place from program flash; only `.data` is copied
  flash → SRAM at boot. The TCMs (ITCM/DTCM) are declared by the SoC but nothing
  is placed there by default — 0 bytes used on both nodes.
- **IVT_HEADER (256 B @ `0x0040_0000`):** the Image Vector Table the BootROM
  reads at reset to locate the application entry point; always 100% (one header).
- **IDT_LIST (32 KB):** build-time scratch for interrupt-table generation; not
  present in the final image.
- **Silicon vs. emulation:** the Renode `s32k388` model provides larger flash
  (8 MB) and SRAM (3×256 KB) at the same base addresses, so the smaller K344
  image fits and boots. Emulated per-core TCM sizes differ from K344 but are
  moot because the TCMs are unused.
- **Inspect it yourself:** `west build` prints the region table on every link;
  `build/<app>/zephyr/zephyr.map` shows per-symbol placement (e.g. `ae_w0` in
  `.rodata`/FLASH, `leak_heap` in SRAM); `linker.cmd` holds the generated
  addresses from the devicetree.
