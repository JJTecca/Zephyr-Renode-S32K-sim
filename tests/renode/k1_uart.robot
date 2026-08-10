*** Settings ***
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${REPL}     ${CURDIR}/../../sim/renode/k1_edge.repl
${ELF}      ${CURDIR}/../../build/k1_powertrain/zephyr/zephyr.elf
${HOOKS}    ${CURDIR}/../../sim/renode/fault_hooks.py
${UART}     sysbus.lpuart2

*** Keywords ***
Boot K1
    Execute Command           mach create "k1"
    Execute Command           machine LoadPlatformDescription @${REPL}
    Execute Command           sysbus LoadELF @${ELF}
    Execute Command           cpu0 VectorTableOffset `sysbus GetSymbolAddress "_vector_table"`
    Create Terminal Tester    ${UART}

*** Test Cases ***
K1 Boots And Streams Telemetry
    Boot K1
    Start Emulation
    Wait For Line On Uart     K1,boot,node=1
    Wait For Line On Uart     TELEM,

Injected Leak Drains The Heap
    Boot K1
    Execute Command           i @${HOOKS}
    Start Emulation
    Wait For Line On Uart     TELEM,
    Execute Command           inject_memory_leak k1 128
    # heap_used (node 1, signal 2) only climbs past 4000 once the leak has
    # eaten most of the 8112-byte heap -- proves the injection actually bit.
    Wait For Line On Uart     TELEM,\\d+,1,2,\\d+,[4-9][0-9]{3}    treatAsRegex=true    timeout=30
