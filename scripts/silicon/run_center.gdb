set pagination off
set tcp auto-retry on
set tcp connect-timeout 240
set remotetimeout 60
target remote :7324
load
monitor reset
continue