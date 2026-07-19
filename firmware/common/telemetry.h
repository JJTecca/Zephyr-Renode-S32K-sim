/*
 * telemetry.h — the frozen on-wire telemetry schema (ADR-007) shared by all
 * nodes. Will define: CAN IDs (0x100 | node_id), node/signal enums
 * (heap_free, heap_used, can_tx_err, can_rx_err, loop_latency), the 8-byte
 * packed telemetry frame (node, signal, seq, value — timestamp added at RX,
 * label + true_ttf added host-side by the campaign), and the fault-control
 * block `sdv_fault_ctl` (magic, leak_bytes_per_tick, busy_spin_us) that the
 * Renode injector writes in virtual time. Plain C, no Zephyr dependencies.
 */
