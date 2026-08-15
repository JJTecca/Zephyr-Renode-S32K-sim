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

CAN Is Unavailable But Boot Survives
    [Documentation]    Negative path: CAN can't init in sim -> must degrade, not abort.
    Boot K1
    Start Emulation
    Wait For Line On Uart     K1,link,ok
    Wait For Line On Uart     K1,can,unavailable_sim
    Wait For Line On Uart     TELEM,

Healthy Node Does Not Leak
    [Documentation]    Negative path: with no fault, heap_used (signal 2) stays 0.
    Boot K1
    Start Emulation
    Wait For Line On Uart     TELEM,
    Should Not Be On Uart     TELEM,\\d+,1,2,\\d+,[1-9]    treatAsRegex=true    timeout=8

Injected Leak Drains The Heap
    Boot K1
    Execute Command           i @${HOOKS}
    Start Emulation
    Wait For Line On Uart     TELEM,
    Execute Command           inject_memory_leak k1 128
    Wait For Line On Uart     TELEM,\\d+,1,2,\\d+,[4-9][0-9]{3}    treatAsRegex=true    timeout=30

Injected Busy Spin Raises Loop Latency
    Boot K1
    Execute Command           i @${HOOKS}
    Start Emulation
    Wait For Line On Uart     TELEM,
    Execute Command           inject_busy_spin k1 5000
    # loop_latency (signal 5) climbs past 50 ms
    Wait For Line On Uart     TELEM,\\d+,1,5,\\d+,([5-9][0-9]|[1-9][0-9]{2,})    treatAsRegex=true    timeout=30