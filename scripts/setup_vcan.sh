# setup_vcan.sh — host virtual CAN bring-up (Linux/WSL2):
# modprobe vcan; ip link add dev vcan0 type vcan; ip link set up vcan0.
# Prerequisite for the SocketCAN bridge in boot_topology.resc, which makes
# emulated frames visible to candump / Wireshark / python-can on the host.
