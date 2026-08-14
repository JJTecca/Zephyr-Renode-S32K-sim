echo "Starting to compile project and generating .elf files K1 & K3"
./s32k1k3_build_os.ps1
Start-Sleep -Seconds 3
echo "Opening Renode Simulator via edge and main hub elf files"
echo "Paste the following :"
echo "Clear
    i @D:/zephyr-ws/Zephyr-Renode-S32K-sim/sim/renode/boot_topology.resc
    i @D:/zephyr-ws/Zephyr-Renode-S32K-sim/sim/renode/fault_hooks.py
    inject_memory_leak k1_powertrain 128
    Clear"
python scripts/renode_open.py
Start-Sleep -Seconds 15
echo "Running campaign & generating .csv logs"
python sim\run_campaign.py --log D:\zephyr-ws\Zephyr-Renode-S32K-sim\k1_telem.log
Start-Sleep -Seconds 2
echo "Running IsoForest alg & generating .csv file"
python ml\dataset.py  --glob "datasets/*.csv"
python ml\baseline.py --glob "datasets/memory_leak_*.csv"
Pause

