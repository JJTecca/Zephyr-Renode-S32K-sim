# fault_hooks.py — deterministic fault injection, included by
# boot_topology.resc. `mc_`-prefixed functions become Renode monitor commands.
# Will provide: inject_memory_leak <machine> <bytes_per_tick>,
# inject_busy_spin <machine> <spin_us>, clear_faults <machine> — each locates
# the target node's `sdv_fault_ctl` symbol via sysbus GetSymbolAddress and
# writes the field + arming magic (0xFA17C0DE) with WriteDoubleWord, in
# VIRTUAL time → reproducible. Only faults with analytical ground truth
# (hard rule). Manual monitor fallback: mach set "<node>";
# $addr = `sysbus GetSymbolAddress "sdv_fault_ctl"`; sysbus WriteDoubleWord ...
