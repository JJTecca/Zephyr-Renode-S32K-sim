*** Settings ***
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

# Headless, parametrized twin of the manual run_all.ps1 paste block.
# Boots ONLY K1 (the telemetry source) and injects a fault at a seed-derived
# onset. K3/hub/CAN and serial-execution are intentionally omitted: they are pure
# overhead for data generation and made each run crawl. Driven by run_all_ci.ps1
# / the Dataset workflow via environment variables:
#   CAMPAIGN_LOG    - path the K1 telemetry is written to
#   CAMPAIGN_INJECT - e.g. "inject_memory_leak k1_powertrain 128" or "inject_busy_spin k1_powertrain 20000"
#   CAMPAIGN_ONSET  - baseline seconds before injecting (varies per seed -> distinct episode)
#   CAMPAIGN_DRAIN  - seconds to run after injecting (long enough to reach OOM)

*** Variables ***
${REPL}      ${CURDIR}/../../sim/renode/k1_edge.repl
${ELF}       ${CURDIR}/../../build/k1_powertrain/zephyr/zephyr.elf
${HOOKS}     ${CURDIR}/../../sim/renode/fault_hooks.py
${LOG}       %{CAMPAIGN_LOG}
${INJECT}    %{CAMPAIGN_INJECT}
${ONSET}     %{CAMPAIGN_ONSET=2}
${DRAIN}     %{CAMPAIGN_DRAIN=12}

*** Test Cases ***
Generate Campaign Dataset
    Execute Command    mach create "k1_powertrain"
    Execute Command    machine LoadPlatformDescription @${REPL}
    Execute Command    sysbus LoadELF @${ELF}
    Execute Command    cpu0 VectorTableOffset `sysbus GetSymbolAddress "_vector_table"`
    Execute Command    sysbus.lpuart2 CreateFileBackend @${LOG}
    Execute Command    i @${HOOKS}

    # baseline -> inject -> drain (single machine, default quantum -> fast)
    Execute Command    emulation RunFor "${ONSET}"
    Execute Command    mach set "k1_powertrain"
    Execute Command    ${INJECT}
    Execute Command    emulation RunFor "${DRAIN}"
