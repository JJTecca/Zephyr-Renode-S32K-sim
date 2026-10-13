set pagination off
set tcp auto-retry on
set tcp connect-timeout 240
set remotetimeout 60
target remote :7224
monitor reset
tbreak main
continue
set var sdv_fault_ctl.busy_spin_us = 5000
set var sdv_fault_ctl.magic = 0xFA17C0DE
detach
quit