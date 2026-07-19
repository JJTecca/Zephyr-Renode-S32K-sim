/*
 * k1_edge main.c — sense-only node; NEVER actuates (hard rule).
 * Will do, every 100 ms tick: poll the volatile `sdv_fault_ctl` block (armed
 * by Renode in virtual time) and, if the memory-leak fault is armed, leak
 * leak_bytes_per_tick from a dedicated K_HEAP — true time-to-OOM stays
 * analytical: ttf = free_bytes / leak_rate. Then read that heap's runtime
 * stats and TX heap_free / heap_used / loop_latency telemetry frames on CAN
 * (chosen zephyr,canbus = flexcan0). Boot + errors printed as single printk
 * lines for the Renode log parser.
 */
