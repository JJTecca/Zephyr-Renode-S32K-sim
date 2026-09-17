# !!! VERY IMPORTANT CLOCK CHANGE — S32K144EVB-Q100 bring-up

**Why this exists:** NXP never shipped a clock config for the S32K144, so our board
builds against the die-compatible `SOC_S32K146` (see `Kconfig.s32k144evb`). The K146
clock recipe drives the core from an **8 MHz external crystal → SPLL → 80 MHz**, and
writing the SOSC/SPLL registers **faults on the K144EVB** (imprecise bus fault, traced
to `Clock_Ip_ClockInitializeObjects`). This patch makes the clock run off the chip's
**internal FIRC oscillator** instead — it never touches the SOSC/SPLL registers, so it
can't hit that fault.

**Attempt #1 — diagnostic:** success = the PEmicro `BusFault` line disappears on reset
(clock init passed). Console output may still need a follow-up (peripheral clock source),
but the fault clearing proves the direction.

## Target file (in the west-managed `hal_nxp` module — NOT this repo)
```
D:\zephyr-ws\modules\hal\nxp\s32\soc\s32k146\src\Clock_Ip_Cfg.c
```
> ⚠️ `west update` overwrites this file. Re-apply these edits (from here) after any update.
> This affects any `s32k146` build too — we only build the K144, so that's fine.

## The 3 edits

**1) Skip external-oscillator (SOSC) init — line ~115**
```c
        1U,                       /* xoscsCount */      // BEFORE
```
```c
        0U,   /* xoscsCount */    /* !!! VERY IMPORTANT CLOCK CHANGE: skip SOSC (no crystal) */
```

**2) Skip PLL (SPLL) init — line ~116**
```c
        1U,                       /* pllsCount */       // BEFORE
```
```c
        0U,   /* pllsCount */     /* !!! VERY IMPORTANT CLOCK CHANGE: skip SPLL */
```

**3) Run the system clock from FIRC instead of SPLL — line ~214**
This is the input source of the **first** selector (`SCS_RUN_CLK`):
```c
            {
                SCS_RUN_CLK,                    /* Clock name associated to selector */
                SPLL_CLK,                       /* Name of the selected input source */   // BEFORE
            },
```
```c
            {
                SCS_RUN_CLK,                    /* Clock name associated to selector */
                FIRC_CLK,   /* !!! VERY IMPORTANT CLOCK CHANGE: run core from internal FIRC */
            },
```

## Rebuild + test
```powershell
cd D:\zephyr-ws\Zephyr-Renode-S32K-sim
west build -b s32k144evb firmware\board_check -d build\bcheck -p always -- -DBOARD_ROOT=D:/zephyr-ws/Zephyr-Renode-S32K-sim
```
Flash `build\bcheck\zephyr\zephyr.elf`, reset, and check the PEmicro log:
- **No `BusFault`** → clock init passed. 🎉 Next: get console output (may need the LPUART
  clock source pointed at a FIRC divider).
- **Still `BusFault`** → revert and we go the RTD 2.0.0 + Config Tools route.
