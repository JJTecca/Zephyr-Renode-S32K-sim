set pagination off
set non-stop on
set tcp auto-retry on
set tcp connect-timeout 240
set remotetimeout 60
target remote :7224
load
monitor reset
continue &
shell ping -n 31 127.0.0.1 >nul
interrupt
set var sdv_fault_ctl.leak_bytes_per_tick = 256
set var sdv_fault_ctl.magic = 0xFA17C0DE
continue &
shell ping -n 31 127.0.0.1 >nul
detach
quit