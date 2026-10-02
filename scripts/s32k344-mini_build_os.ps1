Write-Output "Real sillicon boards build: no Renode simulation, switch to the other ps1"
Write-Output "Build for S32K344-mini EVB (FRDM-A) starting up..."
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim
Remove-Item -Recurse -Force build\k1_frdma -ErrorAction SilentlyContinue
# FRDM-A = CAN sender (K1 edge) on flexcan0 / J15, pairs with the T-box receiver.
west build -b s32k344mini .\firmware\k1_edge -d build\k1_frdma -p always -- -DBOARD_ROOT=D:/zephyr-ws/Zephyr-Renode-S32K-sim -DCONFIG_SDV_NODE_ID=1

Pause
