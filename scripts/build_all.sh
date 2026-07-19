# build_all.sh — one command from zero to ELFs. Will do:
# west init -l . (if no workspace yet) && west update, then
#   west build -b mr_canhubk3/s32k344 firmware/k3_hub        -d build/k3_hub
#   west build -b mr_canhubk3/s32k344 firmware/k1_edge       -d build/k1_powertrain
#   west build -b mr_canhubk3/s32k344 firmware/k1_edge       -d build/k1_chassis \
#     -- -DCONFIG_SDV_NODE_ID=2
# Output paths are exactly what boot_topology.resc LoadELFs.
