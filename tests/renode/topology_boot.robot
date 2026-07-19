# topology_boot.robot — Renode Robot-Framework smoke test, run by
# `renode-test tests/renode/topology_boot.robot`. Will assert:
# (1) boot_topology.resc loads, (2) "Booting Zephyr" then "K3,boot,ok" appear
# on the hub's lpuart2 within virtual-time budget, (3) K1 boot lines appear,
# (4) hub logs TELEM lines from both K1 nodes → CAN path alive end-to-end,
# (5) after inject_memory_leak, heap_free telemetry decreases monotonically.
