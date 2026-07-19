---
tags: [roadmap]
---
# Roadmap July–December (corrected for actual availability)

Ground rule from [[ADR-010 — December scope cut]]: a **complete thin loop beats a wide pile of halves.**
Two regimes, not one: **summer (July–September) is a full push** — every deep tool lands here;
**October–December is part-time** — shallow tools only, integration and measurement, no new deep learning.

## July — FULL PUSH · rig + brain
Python 3.13 + uv + NumPy/pandas. Vault + Git automation live. Start [[Renode]]: 2×K1 + 1×K3 on [[vcan]]
(the Renode↔vcan bridge is the riskiest integration — test it FIRST). Choose RTOS (ADR — [[FreeRTOS]] vs [[Zephyr]], never revisit).
**Telemetry schema FROZEN**: (timestamp, signal, value, label, true_time_to_failure).
Exit: telemetry flows end-to-end in sim; you can compute a 30-second rolling heap slope without googling.

## August — FULL PUSH · data
[[Renode]] hooks. [[PyTorch]] + the [[denoising autoencoder]]. [[scikit-learn]]. [[SocketCAN]]. [[DVC]].
[[fault injection]] scripts; labelled dataset v1 (DVC-tracked). [[Isolation Forest]] + AE baselines on [[MIMII]] / [[C-MAPSS]].
Exit: dataset card written; baseline ROC per class.
**GATE: verify Renode's FlexCAN exposes TEC/REC → bus-off. If not, swap that flagship for timing/deadline-miss NOW** ([[ADR-011 — Fault taxonomy resolved for December]]).

## September — FULL PUSH · the detector, on target
[[quantization]] (int8 PTQ + calibration set). [[LiteRT for Microcontrollers]]. [[litert-torch]] (+ parity check).
[[FreeRTOS]] schedule design (supervisor always preempts inference). **Start [[PyTorch Geometric]]** — the headline
must not sit on the thinnest slice of time. Exit: **THE VERTICAL SLICE** — injected fault → [[int8]] inference on
“S32K3-class (S32K388)” in Renode (then CANHUBK344 if it arrived) → anomaly score over UART, with measured ms and KB.
**This is the go/no-go for December**: if the slice isn't running, cut the GNN to a demo and protect the closed loop — decide here, not in November.

## October — PART-TIME · cloud backbone
Only shallow tools: [[MQTT]] → [[Redpanda]] → [[InfluxDB]]/[[TimescaleDB]] → [[FastAPI]] → [[Grafana]] on [[Railway]];
incident log in [[pgvector]]. **Finish PyG**: [[GNN]] v0 (2-layer GCN, 5-node graph), advisory mode.
Exit: injected fault visible on the dashboard within seconds; GNN emits a propagation proposal.

## November — PART-TIME · close the loop + measure (ZERO new tools by design)
[[ISO 26262]] / [[SOTIF]] argument (reading, not coding). [[safety supervisor]] whitelist in C.
**ONE fault class ([[memory leak]]) healed end-to-end** by a supervisor-approved [[service restart]]. Metrics campaign.
Exit: [[MTTR]], recovery success and [[false positive rate]]/h measured over ≥30 incidents; supervisor veto count > 0 — proof the [[safety cage]] is live.

## December — PART-TIME · assemble
Typst. Nothing new. Chapters assembled from vault notes. Demo script — a 10-minute demo runnable from one command.
Future work = RL, federated, causal, TSN, S32K5/NPU port. Exit: 5 chapters in draft.

## Jan–Jul 2027 — MARGIN (named as margin, not work)
Second fault class. Physical two-node rig (stretch, [[ADR-012 — Renode is the plan of record]]). IEEE/SAE paper. Polish and defend.
If December slips six weeks, nothing breaks — that is the honest reason this is achievable.
