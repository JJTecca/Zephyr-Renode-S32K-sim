Write-Output "Starting to compile project and generating .elf files K1 & K3"
./s32k1k3_build_os.ps1
Start-Sleep -Seconds 3

Write-Output "Opening Renode Simulator via edge and main hub elf files"
Write-Output "Paste the following :"
Write-Output "Clear
    i @D:/zephyr-ws/Zephyr-Renode-S32K-sim/sim/renode/boot_topology.resc
    i @D:/zephyr-ws/Zephyr-Renode-S32K-sim/sim/renode/fault_hooks.py
    inject_memory_leak k1_powertrain 128
    inject_busy_spin k1_powertrain 5000
    Clear"

python scripts/renode_open.py
Start-Sleep -Seconds 15

$closed = Read-Host "Close Renode, then type YES to generate the CSV logs"
if ($closed -ne "YES") {
    throw "Stopped: close Renode before parsing k1_telem.log."
}

# could change to deadline_miss
# dont forget to change leak rates, manually adjust 64/128/256
$Fault = "memory_leak"
Write-Output "Running campaign & generating .csv logs"
python sim\run_campaign.py --log D:\zephyr-ws\Zephyr-Renode-S32K-sim\k1_telem.log --fault $Fault --rate 256

Start-Sleep -Seconds 2

Write-Output "Running IsoForest alg & generating .csv file"
python ml\dataset.py --glob "datasets/*.csv"
python ml\baseline.py --glob "datasets/memory_leak_*.csv"

if($Fault -eq "memory_leak") {
    Write-Output "Predicting how much time left for K1 to run OOM and crash"
    python ml\predictor.py --glob "datasets/memory_leak_*.csv"
}

Write-Output "Re-run the .csv existing files & generate manifest again"
python .\ml\train_ae.py
Write-Output "Creating int8 .npz file locally"
python ml\quantize.py


Pause