param(
    [ValidateSet("memory_leak", "deadline_miss")]
    [string]$Fault = "memory_leak",
    [int]$Strength = 0   # 0 = class default (leak: 128 bytes/tick, timing: 20000 us)
)

# One fault per run (they clear each other in fault_hooks.py, so never both).
# The inject_* names must match the bare function names in fault_hooks.py.
if ($Fault -eq "memory_leak") {
    if ($Strength -eq 0) { $Strength = 128 }
    $inject = "inject_memory_leak k1_powertrain $Strength"
} else {
    if ($Strength -eq 0) { $Strength = 20000 }
    $inject = "inject_busy_spin k1_powertrain $Strength"
}

Write-Output "=== Fault: $Fault (strength=$Strength) ==="
Write-Output "Compiling project and generating .elf files K1 & K3"
./s32k1k3_build_os.ps1
Start-Sleep -Seconds 3

Write-Output "Opening Renode. Paste the following into the monitor:"
Write-Output "Clear
    i @D:/zephyr-ws/Zephyr-Renode-S32K-sim/sim/renode/boot_topology.resc
    i @D:/zephyr-ws/Zephyr-Renode-S32K-sim/sim/renode/fault_hooks.py
    $inject
    Clear"

python scripts/renode_open.py
Start-Sleep -Seconds 15

$closed = Read-Host "Close Renode, then type YES to generate the CSV"
if ($closed -ne "YES") {
    throw "Stopped: close Renode before parsing k1_telem.log."
}

Write-Output "Labelling the $Fault capture into datasets/"
python sim\run_campaign.py --log D:\zephyr-ws\Zephyr-Renode-S32K-sim\k1_telem.log --fault $Fault
Start-Sleep -Seconds 2

Write-Output "Running IsoForest + AE baseline over ALL datasets/*.csv"
python ml\dataset.py  --glob "datasets/*.csv"
python ml\baseline.py --glob "datasets/*.csv"

Pause
