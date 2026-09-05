*** Settings ***
Documentation     SDV Fault-Prediction sim — single Renode Robot suite.
...               Node-level (K1), K1<->K3 integration, on-target detector, and the
...               Sprint-3 supervisor skeleton (tagged 'pending'). The campaign data
...               generator lives here too, tagged 'campaign' (exclude in CI).
...               Paths are ${CURDIR}-relative so it runs on Windows and in CI.
...               CI:  renode-test tests/renode/sdv_sim.robot --exclude pending --exclude campaign
Suite Setup       Setup
Suite Teardown    Teardown
Test Teardown     Test Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${REPL_K1}     ${CURDIR}/../../sim/renode/k1_edge.repl
${REPL_K3}     ${CURDIR}/../../sim/renode/k3_hub_s32k388.repl
${HOOKS}       ${CURDIR}/../../sim/renode/fault_hooks.py
${ELF_K1}      ${CURDIR}/../../build/k1_powertrain/zephyr/zephyr.elf
${ELF_K3}      ${CURDIR}/../../build/k3_hub/zephyr/zephyr.elf
${CONSOLE}     sysbus.lpuart2
# campaign harness (env-driven); safe defaults so a bare run still works
${LOG}         %{CAMPAIGN_LOG=k1_telem.log}
${INJECT}      %{CAMPAIGN_INJECT=inject_memory_leak k1_powertrain 128}
${ONSET}       %{CAMPAIGN_ONSET=2}
${DRAIN}       %{CAMPAIGN_DRAIN=12}

*** Keywords ***
Boot K1 Node
    [Documentation]    Single K1 machine on its console UART (no bus). Returns tester id.
    [Arguments]    ${name}    ${elf}=${ELF_K1}
    Execute Command    mach create "${name}"
    Execute Command    machine LoadPlatformDescription @${REPL_K1}
    Execute Command    sysbus LoadELF @${elf}
    Execute Command    cpu0 VectorTableOffset `sysbus GetSymbolAddress "_vector_table"`
    ${t}=    Create Terminal Tester    ${CONSOLE}    machine=${name}
    RETURN    ${t}

Boot Bus Machine
    [Documentation]    One machine wired to the shared CAN + UART hub. Returns tester id.
    [Arguments]    ${name}    ${repl}    ${elf}
    Execute Command    mach create "${name}"
    Execute Command    machine LoadPlatformDescription @${repl}
    Execute Command    sysbus LoadELF @${elf}
    Execute Command    cpu0 VectorTableOffset `sysbus GetSymbolAddress "_vector_table"`
    Execute Command    connector Connect sysbus.can0    canbus0
    Execute Command    connector Connect sysbus.lpuart1 uartbus0
    ${t}=    Create Terminal Tester    ${CONSOLE}    machine=${name}
    RETURN    ${t}

Boot Two Node Topology
    [Documentation]    Headless K3 hub + K1 powertrain on a shared CAN + UART hub.
    ...                Returns (k3 tester, k1 tester).
    Execute Command    emulation CreateCANHub  "canbus0"
    Execute Command    emulation CreateUARTHub "uartbus0"
    ${k3}=    Boot Bus Machine    k3_hub          ${REPL_K3}    ${ELF_K3}
    ${k1}=    Boot Bus Machine    k1_powertrain   ${REPL_K1}    ${ELF_K1}
    Execute Command    emulation SetGlobalQuantum "0.000025"
    Execute Command    emulation SetGlobalSerialExecution True
    RETURN    ${k3}    ${k1}

Load Fault Hooks
    Execute Command    i @${HOOKS}

Inject Memory Leak
    [Arguments]    ${machine}    ${rate}
    Execute Command    mach set "${machine}"
    Execute Command    inject_memory_leak ${machine} ${rate}

Inject Busy Spin
    [Arguments]    ${machine}    ${us}
    Execute Command    mach set "${machine}"
    Execute Command    inject_busy_spin ${machine} ${us}

*** Test Cases ***
K1 Boots And Streams Telemetry
    [Tags]    node
    Boot K1 Node    k1
    Start Emulation
    Wait For Line On Uart    K1,boot,node=1
    Wait For Line On Uart    TELEM,

CAN Degrades But Boot Survives
    [Documentation]    Negative: CAN can't init in sim -> degrade, not abort.
    [Tags]    node
    Boot K1 Node    k1
    Start Emulation
    Wait For Line On Uart    K1,link,ok
    Wait For Line On Uart    K1,can,unavailable_sim
    Wait For Line On Uart    TELEM,

Healthy Node Does Not Leak
    [Documentation]    Negative: with no fault, heap_used (signal 2) stays 0.
    [Tags]    node
    Boot K1 Node    k1
    Start Emulation
    Wait For Line On Uart    TELEM,
    Should Not Be On Uart    TELEM,\\d+,1,2,\\d+,[1-9]    treatAsRegex=true    timeout=8

Injected Leak Drains The Heap
    [Tags]    node
    Boot K1 Node    k1
    Load Fault Hooks
    Start Emulation
    Wait For Line On Uart    TELEM,
    Inject Memory Leak    k1    128
    Wait For Line On Uart    TELEM,\\d+,1,2,\\d+,[4-9][0-9]{3}    treatAsRegex=true    timeout=30

Injected Busy Spin Raises Loop Latency
    [Tags]    node
    Boot K1 Node    k1
    Load Fault Hooks
    Start Emulation
    Wait For Line On Uart    TELEM,
    Inject Busy Spin    k1    5000
    Wait For Line On Uart    TELEM,\\d+,1,5,\\d+,([5-9][0-9]|[1-9][0-9]{2,})    treatAsRegex=true    timeout=30

Hub And Node Boot On Shared Bus
    [Tags]    integration
    ${k3}    ${k1}=    Boot Two Node Topology
    Start Emulation
    Wait For Line On Uart    K3,boot,ok        testerId=${k3}
    Wait For Line On Uart    K1,boot,node=1    testerId=${k1}

Hub Receives Node Telemetry And Scores It
    [Documentation]    Transport alive: K1 frames reach K3, K3 runs the detector.
    [Tags]    integration    detector
    ${k3}    ${k1}=    Boot Two Node Topology
    Start Emulation
    Wait For Line On Uart    K3,rx,node=1    testerId=${k3}    timeout=30
    Wait For Line On Uart    K3,score,seq=\\d+,score=\\d+\\.\\d+,alarm=[01]
    ...    testerId=${k3}    treatAsRegex=true    timeout=30

Healthy Run Produces No Alarm
    [Documentation]    False-positive guard: no fault -> alarm must stay 0.
    [Tags]    detector
    ${k3}    ${k1}=    Boot Two Node Topology
    Start Emulation
    Wait For Line On Uart    K3,score,             testerId=${k3}    timeout=30
    Should Not Be On Uart    K3,score,\\S+,alarm=1    testerId=${k3}    treatAsRegex=true    timeout=15

Leak Propagates To Hub And Raises Alarm
    [Tags]    integration    detector
    ${k3}    ${k1}=    Boot Two Node Topology
    Load Fault Hooks
    Start Emulation
    Wait For Line On Uart    K3,rx,node=1    testerId=${k3}    timeout=30
    Inject Memory Leak    k1_powertrain    256
    Wait For Line On Uart    K3,score,\\S+,alarm=1    testerId=${k3}    treatAsRegex=true    timeout=60
    Wait For Line On Uart    K3,observer,notify       testerId=${k3}    timeout=60

Supervisor Approves A Whitelisted Heal Before OOM
    [Documentation]    Sprint 3 exit criterion. Enable once the plain-C supervisor
    ...                emits K3,supervisor,* lines; adjust expected text to match.
    [Tags]    sprint3    pending
    ${k3}    ${k1}=    Boot Two Node Topology
    Load Fault Hooks
    Start Emulation
    Wait For Line On Uart    K3,rx,node=1    testerId=${k3}    timeout=30
    Inject Memory Leak    k1_powertrain    256
    Wait For Line On Uart    K3,score,\\S+,alarm=1                  testerId=${k3}    treatAsRegex=true    timeout=60
    Wait For Line On Uart    K3,supervisor,approve,action=RESTART    testerId=${k3}    timeout=60
    Wait For Line On Uart    K3,supervisor,veto_count=[1-9]         testerId=${k3}    treatAsRegex=true    timeout=60

Generate Campaign Dataset
    [Documentation]    DATA-GEN HARNESS, not a pass/fail test. Env-driven
    ...                (CAMPAIGN_LOG/INJECT/ONSET/DRAIN); used by run_all_ci.ps1 /
    ...                dataset.yml. Boots only K1 and writes a UART file backend.
    [Tags]    campaign
    Execute Command    mach create "k1_powertrain"
    Execute Command    machine LoadPlatformDescription @${REPL_K1}
    Execute Command    sysbus LoadELF @${ELF_K1}
    Execute Command    cpu0 VectorTableOffset `sysbus GetSymbolAddress "_vector_table"`
    Execute Command    sysbus.lpuart2 CreateFileBackend @${LOG}
    Load Fault Hooks
    Execute Command    emulation RunFor "${ONSET}"
    Execute Command    mach set "k1_powertrain"
    Execute Command    ${INJECT}
    Execute Command    emulation RunFor "${DRAIN}"
