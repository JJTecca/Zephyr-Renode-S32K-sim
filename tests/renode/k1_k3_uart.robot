*** Settings ***
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

# Headless, parametrized twin of the manual run_all.ps1 paste block.
# Boots K3 + K1 on the UART hub (same as boot_topology.resc), captures K1
# telemetry to a file, injects a fault at a seed-derived onset, and runs to OOM.
# Driven by run_all_ci.ps1 via environment variables:
#   CAMPAIGN_LOG    - path the K1 telemetry is written to
#   CAMPAIGN_INJECT - e.g. "inject_memory_leak k1_powertrain 128" or "inject_busy_spin k1_powertrain 20000"
#   CAMPAIGN_ONSET  - baseline seconds before injecting (varies per seed -> distinct episode)
#   CAMPAIGN_DRAIN  - seconds to run after injecting (long enough to reach OOM)

*** Variables ***
${K1REPL}    ${CURDIR}/../../sim/renode/k1_edge.repl
${K3REPL}    ${CURDIR}/../../sim/renode/k3_hub_s32k388.repl
${K1ELF}     ${CURDIR}/../../build/k1_powertrain/zephyr/zephyr.elf
${K3ELF}     ${CURDIR}/../../build/k3_hub/zephyr/zephyr.elf
${HOOKS}     ${CURDIR}/../../sim/renode/fault_hooks.py
${LOG}       %{CAMPAIGN_LOG}
${INJECT}    %{CAMPAIGN_INJECT}
${ONSET}     %{CAMPAIGN_ONSET=2}
${DRAIN}     %{CAMPAIGN_DRAIN=12}

*** Test Cases ***
Generate Campaign Dataset
    Execute Command    emulation CreateCANHub "canbus0"
    Execute Command    emulation CreateUARTHub "uartbus0"

    # --- K3 zonal hub ---
    Execute Command    mach create "k3_hub"
    Execute Command    machine LoadPlatformDescription @${K3REPL}
    Execute Command    sysbus LoadELF @${K3ELF}
    Execute Command    cpu0 VectorTableOffset `sysbus GetSymbolAddress "_vector_table"`
    Execute Command    connector Connect sysbus.can0 canbus0
    Execute Command    connector Connect sysbus.lpuart1 uartbus0

    # --- K1 edge (sender) ---
    Execute Command    mach create "k1_powertrain"
    Execute Command    machine LoadPlatformDescription @${K1REPL}
    Execute Command    sysbus LoadELF @${K1ELF}
    Execute Command    cpu0 VectorTableOffset `sysbus GetSymbolAddress "_vector_table"`
    Execute Command    connector Connect sysbus.can0 canbus0
    Execute Command    connector Connect sysbus.lpuart1 uartbus0
    Execute Command    sysbus.lpuart2 CreateFileBackend @${LOG}
    Execute Command    i @${HOOKS}

    Execute Command    emulation SetGlobalQuantum "0.000025"
    Execute Command    emulation SetGlobalSerialExecution True

    # --- baseline -> inject -> drain ---
    Execute Command    emulation RunFor "${ONSET}"
    Execute Command    mach set "k1_powertrain"
    Execute Command    ${INJECT}
    Execute Command    emulation RunFor "${DRAIN}"
