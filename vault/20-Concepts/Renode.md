---
tags: [concept]
---
# Renode

Antmicro's system simulator; runs unmodified firmware for multi-node networks and enables deterministic [[fault injection]] via hooks (peripheral read/write, CPU, timed) in **virtual time** — same seed → byte-identical logs, which IS the reproducibility claim. Backbone of [[ADR-006 — Fault data is generated, reproducibly]] and [[ADR-012 — Renode is the plan of record]]. Caveat: no S32K344 model — use S32K388 (“S32K3-class (S32K388)”). The FlexCAN model was verified by Antmicro against Zephyr samples.
