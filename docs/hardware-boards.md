# Hardware boards on hand — what runs our firmware, and how to wire it safely

Interim hardware note while the target board (**MR-CANHUBK344**, S32K344 Cortex-M7)
is on order (delivery end Oct / early Nov 2026). Everything below is **safe by
construction** — no step here can damage a board if followed. Exact header/pin
numbers must be read off each board's own schematic / quick-start guide (linked);
this note gives the silicon-level rules, which do not change.

Baseline reminder: the whole thesis runs in **Renode with no board**
(`docs/simulation-coverage.md`). None of these boards are required — they add
on-target realism, not a new claim. Do not block on hardware.

---

## What we build for

Firmware target is `mr_canhubk3/s32k344` (S32K344, Cortex-M7), Zephyr **v4.2.0**,
one ELF for sim and silicon. Console = **lpuart2**; inter-node link = **lpuart1**
(ADR-017, the sim's stand-in for CAN FD). A board only runs our binary unchanged
if it is *the same MCU with the same board definition*; a different S32K344 board
needs its own Zephyr board port (XTAL, clocks, LPUART pinmux, console), and a
different **core** (S32K1 = Cortex-M4F) cannot run the M7 image at all.

## Boards identified

| Board (photo) | MCU | Core | Onboard debugger |
|---|---|---|---|
| **S32K3-T-BOX** | S32K344 | Cortex-M7 (lockstep) | No — external SWD (PEmicro/J-Link) on JTAG header |
| **S32K344MINI-EVB** ("TLALOC", 2025) | S32K344 | Cortex-M7 | On-board USB debug (per MINI-EVB family) |
| **S32K144EVB-Q100** (gold) | S32K144 | Cortex-M4F | OpenSDA (USB VCOM) |
| **S32K148EVB-Q176** (black, SCH-53148) | S32K148 | Cortex-M4F | OpenSDA (USB VCOM) |

## Can we run them?

| Board | Zephyr v4.2.0 board upstream? | Runs our firmware? | Verdict |
|---|---|---|---|
| S32K344MINI-EVB | ✗ (too new, 2025) | Same M7 silicon → **yes, after a board port** | **Best interim target.** Closest analog to MR-CANHUBK344: identical S32K344 MCU. |
| S32K3-T-BOX | ✗ | Same M7 silicon → **yes, after a board port** (more work) | Usable but heavier (telematics peripherals, external debugger, 12 V rail). Lower priority. |
| S32K144EVB-Q100 | ✗ (no `s32k144` in Zephyr; NuttX only) | **No** — wrong core (M4F), no board | Not usable for our image as-is. |
| S32K148EVB-Q176 | ✓ `s32k148_evb` | **No** for the M7 image; **yes if rebuilt** `-b s32k148_evb` as a **K1 edge node only** | K1 (sense-only) ports here; K3 detector wants the M7. |

Upstream Zephyr S32K boards: only `mr_canhubk3/s32k344` (M7), `s32k148_evb` (M4F),
`ucans32k1sic` (S32K146, M4F). There is **no** upstream board for S32K144, the
S32K344MINI-EVB, the T-BOX, or a bare S32K388 (`s32k388` exists only as a Renode
*CPU model*, which is what our sim already uses).

**Renode:** unchanged. The sim runs the S32K344 ELF on the upstream S32K388 model
with no board. These physical boards do not alter the Renode flow; keep developing
in sim exactly as now.

## Recommended interim path (before MR-CANHUBK344 arrives)

1. **Primary — S32K344MINI-EVB.** Add an out-of-tree Zephyr board that reuses the
   S32K344 SoC + `hal_nxp` and copies `mr_canhubk3`'s structure, changing only the
   MINI-EVB specifics (external/internal oscillator frequency, LPUART pin routing,
   console UART, LED). Build `firmware/k3_hub` and `firmware/k1_edge` against it.
   This is the low-risk way to get one physical S32K344 node running our real code.
2. **Two-node bench** (K1 ↔ K3) needs two S32K344 boards — pair the MINI-EVB with
   the T-BOX (also S32K344) once the MINI-EVB port is proven.
3. **Optional K1 realism** on `s32k148_evb`: rebuild only `k1_edge` (`-b s32k148_evb`);
   it is sense-only and its S32K1 FlexCAN + LPUART map cleanly. The M4F cannot host
   the K3 autoencoder — keep K3 on S32K344.
4. Keep the gold **S32K144EVB** for GPIO/analog experiments; it is not on our
   Zephyr path.

---

## Safe wiring — two-node UART link (K1 ↔ K3)

The one photo already shows boards cabled up. To run *our* topology on two S32K344
boards you only need the **lpuart1 link** crossed between them and a shared ground;
consoles come out each board's own USB.

**Signals (3.3 V logic, all S32K I/O):**

```
Board A (K3 hub)                 Board B (K1 edge)
  lpuart1 TX  ───────────────►   lpuart1 RX
  lpuart1 RX  ◄───────────────   lpuart1 TX
  GND         ───────────────    GND        (single common ground)
```

- **Cross TX↔RX**, straight **GND↔GND**. That is the entire link.
- **Console per board:** lpuart2 → each board's own onboard debugger USB VCOM
  (or its lpuart2 header) → separate terminal @ 115200 8N1. Do not merge consoles.
- Match baud on both ends (link + console = 115200 8N1, per the app overlays).

### Hard safety rules (no step here can damage a board)

- **Never tie power rails together.** No board's 5 V / 3.3 V / VBAT to another
  board's power pin. Each board powers itself from its **own** USB. Joining power
  outputs back-feeds regulators and is the one thing that fries boards — the UART
  link carries **signal + GND only**.
- **Ground before signal.** Connect GND first, then TX/RX. Never hot-plug a signal
  wire to an unpowered/floating board.
- **Stay at 3.3 V.** All four boards' MCU I/O is 3.3 V. Do not drive 5 V into any
  UART pin. If a level shifter/adapter is in the path, confirm it is set to 3.3 V.
- **T-BOX power:** power it only from USB or its specified 12 V supply per its
  manual. Do **not** apply automotive/battery voltage on the harness connectors
  during UART bring-up, and never share that 12 V/VBAT domain with another board.
- **Flashing is safe** via the onboard USB debugger (OpenSDA on the S32K1 EVBs;
  onboard USB on the MINI-EVB) or an external SWD probe (PEmicro/J-Link) on the
  T-BOX JTAG header. Use one probe per board; do not cross-connect debug and power.
- One board's TX drives the other's RX only — never wire two TX pins together.

---

## Sources

- [MR-CANHUBK3 (Zephyr board, S32K344 M7)](https://docs.zephyrproject.org/latest/boards/nxp/mr_canhubk3/doc/index.html)
- [S32K148EVB-Q176 (Zephyr board, S32K148 M4F)](https://docs.zephyrproject.org/latest/boards/nxp/s32k148_evb/doc/index.html)
- [UCANS32K1SIC (Zephyr board, S32K146 M4F)](https://docs.zephyrproject.org/latest/boards/nxp/ucans32k1sic/doc/index.html)
- [NXP Zephyr boards index](https://docs.zephyrproject.org/latest/boards/nxp/index.html)
- [S32K344-WB / S32K344MINI-EVB (NXP)](https://www.nxp.com/design/design-center/development-boards-and-designs/S32K344-WB)
- [S32K3-T-BOX reference design (NXP)](https://nxp.com/design/designs/s32k3-automotive-telematics-box-t-box-reference-design-board:S32K3-T-BOX)
- [S32K144EVB-Q100 (NXP)](https://www.nxp.com/part/S32K144EVB-Q100)
- [Testing NXP S32K in Renode with Zephyr (Zephyr Project)](https://www.zephyrproject.org/testing-nxp-s32k-automotive-general-purpose-mcu-in-renode-simulation-with-zephyr-rtos/)
