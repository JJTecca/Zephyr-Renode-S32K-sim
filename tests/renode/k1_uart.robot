*** Settings ***
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${REPL}    ${CURDIR}/../../sim/renode/k1_edge.repl
${ELF}     ${CURDIR}/../../build/k1_powertrain/zephyr/zephyr.elf
${UART}    sysbus.lpuart2

*** Test Cases ***
K1 Boots And Streams Telemetry
    Execute Command           mach create "k1"
    Execute Command           machine LoadPlatformDescription @${REPL}
    Execute Command           sysbus LoadELF @${ELF}
    Execute Command           cpu0 VectorTableOffset `sysbus GetSymbolAddress "_vector_table"`
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     K1,boot,node=1
    Wait For Line On Uart     TELEM,