def mc_inject_memory_leak(machine_name, rate):
    bus  = monitor.Machine.SystemBus
    addr = bus.GetSymbolAddress("sdv_fault_ctl")
    bus.WriteDoubleWord(addr,     0xFA17C0DE)   # magic  -> armed
    bus.WriteDoubleWord(addr + 4, int(rate))    # leak_bytes_per_tick

def mc_clear_faults(machine_name):
    bus  = monitor.Machine.SystemBus
    addr = bus.GetSymbolAddress("sdv_fault_ctl")
    bus.WriteDoubleWord(addr, 0x0)              # magic = 0 -> disarmed