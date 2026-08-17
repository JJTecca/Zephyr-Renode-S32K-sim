def _bus_addr():
    bus = monitor.Machine.SystemBus # type: ignore
    #renode type of variable
    return bus, bus.GetSymbolAddress("sdv_fault_ctl")

def mc_nject_memory_leak(machine_name, rate):
    bus, a = _bus_addr()
    bus.WriteDoubleWord(a,     0xFA17C0DE)   # magic -> armed
    bus.WriteDoubleWord(a + 4, int(rate))    # leak_bytes_per_tick
    bus.WriteDoubleWord(a + 8, 0)            # timing off

def mc_inject_busy_spin(machine_name, us):
    bus, a = _bus_addr()
    bus.WriteDoubleWord(a,     0xFA17C0DE)   # magic -> armed
    bus.WriteDoubleWord(a + 4, 0)            # leak off
    bus.WriteDoubleWord(a + 8, int(us))      # busy_spin_us ramp per tick

def mc_clear_faults(machine_name):
    bus, a = _bus_addr()
    bus.WriteDoubleWord(a,     0x0)          # magic = 0 -> disarmed (heals)
    bus.WriteDoubleWord(a + 4, 0)
    bus.WriteDoubleWord(a + 8, 0)