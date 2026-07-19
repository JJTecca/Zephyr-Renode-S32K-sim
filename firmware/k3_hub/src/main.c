/*
 * k3_hub main.c — the on-vehicle loop host: SENSE_RX → DETECT → PREDICT →
 * SUPERVISOR(veto) → ACT. Day-one scope: SENSE_RX only — CAN RX filter on the
 * telemetry ID block (0x100–0x10F) into a message queue, then each frame is
 * timestamped and printed as one CSV line on the console UART (lpuart2 in
 * Renode): `TELEM,<uptime_ms>,<node>,<signal>,<seq>,<value>` — the stream
 * run_campaign.py parses into the labelled dataset. Also TX a 1 Hz heartbeat.
 * Next milestones, in order: detect (int8, CMSIS-NN), supervisor veto
 * (plain C, whitelist restart/degraded/load-shed, always preempts inference),
 * uplink.
 */
