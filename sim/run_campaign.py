# run_campaign.py — the dataset factory, one command:
#   uv run sim/run_campaign.py --config sim/configs/memory_leak.yaml --seed 42
# Will do: launch Renode headless with boot_topology.resc, arm the injection
# from the YAML config at its virtual-time offset, capture the hub's UART
# TELEM,... CSV stream, join with the injection schedule to attach (label,
# true_time_to_failure), and emit one labelled CSV in the frozen ADR-007
# schema (timestamp, signal, value, label, true_ttf) under datasets/.
# Same seed → byte-identical output: the reproducibility claim, asserted by CI.
