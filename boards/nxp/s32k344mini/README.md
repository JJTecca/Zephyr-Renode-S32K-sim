# s32k344mini — S32K344MINI-EVB / FRDM-A-S32K344 (K3 hub)

Out-of-tree Zephyr board for the NXP **S32K344MINI-EVB** (renamed **FRDM-A-S32K344**),
used as the **K3 hub** on real silicon. Same S32K344 Cortex-M7 as the incoming
MR-CANHUBK344, so it runs `firmware/k3_hub` unchanged.

Cloned from upstream Zephyr v4.2.0 `boards/nxp/mr_canhubk3` (same SoC), so it builds
as-is. Build (from the west workspace, `BOARD_ROOT` = this repo):

```
west build -b s32k344mini firmware/k3_hub -d build/k3_mini -p always -- -DBOARD_ROOT=<repo>
```

Flash: the on-board debugger is a PEmicro **OpenSDA** (not CMSIS-DAP), so `west flash`
runners (pyocd/linkserver) do **not** see it. Flash `build/k3_mini/zephyr/zephyr.elf`
via **S32 Design Studio** (PEmicro), or attach a J-Link to the `JTAG Cortex 20`
header (J9) and use `west flash -r jlink`.

## ⚠️ First-try assumptions — confirm against NXP's MINI-EVB example, then fix here

These are inherited from `mr_canhubk3` and are the values to verify on the real board
(read them from NXP's S32K344MINI-EVB example clock/pin config, or the HW manual):

| Item | Assumed (mr_canhubk3) | Where to fix |
|---|---|---|
| Console UART (OpenSDA VCOM) | `lpuart2` on PTA8/PTA9 | `chosen.zephyr,console` + `lpuart2` pinctrl |
| FXOSC crystal | S32K3 SoC default (~16 MHz) | add an `&fxosc` override in `.dts` if different |
| CAN channel | `flexcan0` (PTA6/PTA7) | `chosen.zephyr,canbus` + `flexcan*` pinctrl |

If the board flashes but the console is silent, the console UART is wrong — point
`zephyr,console` at the LPUART the MINI-EVB routes to its VCOM and rebuild.

**FS26 watchdog:** this board (like the T-BOX) has an FS26 safety PMIC whose watchdog
can reset the MCU. Put the FS26 in **debug mode** (board jumper) during bring-up, or the
firmware may reset repeatedly.
