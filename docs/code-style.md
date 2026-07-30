# Code style — the SDV house convention

Our C file convention: the **labeled star-box** banner (the recognizable
firmware lineage), with one field no upstream style has — `Layer:`, the file's
place in the K1 → K3 architecture. Grounded in the classic *Recommended C Style*
(Indian Hill / SunOS) section ordering and firmware Doxygen practice.

## 1 · File header (star box, 78 cols)

```c
/*****************************************************************************
* File:        telemetry.h
* Description: Frozen on-wire telemetry schema (ADR-007), shared by all nodes.
* Layer:       firmware/common  (shared contract, no Zephyr dependencies)
* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing
* Copyright (c) 2026 Maior Cristian-Alexandru
*****************************************************************************/
```

`Layer:` is **ours** — it names the file's role in the architecture
(`firmware/common`, `firmware/k1_edge`, `firmware/k3_hub`, `sim/...`). Labels are
left-aligned to the width of `Description:`.

## 2 · Section banners (medium star box, 52 cols)

One box per section, name in CAPS:

```c
/***************************************************
* INCLUDE FILES
***************************************************/
```

## 3 · Function header (star box, 78 cols)

```c
/*****************************************************************************
* Function:    send_telem
* Description: Pack a telemetry frame and transmit it on CAN.
* Returns:     0 on success, negative errno on failure.
*****************************************************************************/
```

`Returns:` only when the function returns a value.

## 4 · Section order (use only what a file needs)

| Banner | Holds |
|---|---|
| `INCLUDE FILES` | system headers first, then local (`"..."`) |
| `MACRO DEFINITIONS` | `#define` constants and function-like macros |
| `ENUMERATIONS` | enums |
| `TYPE DEFINITIONS` | typedefs, structs, unions |
| `STATIC DATA` | file-scope variables |
| `FUNCTION PROTOTYPES` | static function prototypes |
| `FUNCTIONS` | function definitions (each with a Function header) |

Never mix enums, macros, and prototypes under one banner — keeping them in
separate labeled boxes is the whole point.

## 5 · Rules of thumb

- Star at **column 0** on every banner line (`* text`), ASCII only (byte-stable).
- File/function boxes are 78 cols; section boxes 52 cols.
- 4-space indent; aligned struct/enum members and label fields.
- A banner earns its place only if the section has content — no empty boxes.
- These boxes are decorative (a `/****` opener isn't a Doxygen block). If we
  later want Doxygen output, switch the opener to `/**` and keep the fields.

Reference implementations: [`firmware/common/telemetry.h`](../firmware/common/telemetry.h),
[`firmware/k1_edge/src/main.c`](../firmware/k1_edge/src/main.c).

Sources: [Recommended C Style and Coding Standards (Indian Hill)](https://cseweb.ucsd.edu/~ricko/CSE30/indhill-cstyle.html) ·
[micro-os-plus Doxygen style guide](https://micro-os-plus.github.io/develop/doxygen-style-guide/) ·
[MaJerle/c-code-style](https://github.com/MaJerle/c-code-style).
