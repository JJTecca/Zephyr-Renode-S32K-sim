Write-Output "Real sillicon boards build: no Renode simulation, switch to the other ps1"
Write-Output "Build for FRDM-A-S32K344 (CAN sender) starting up..."
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim
Remove-Item -Recurse -Force build\frdma -ErrorAction SilentlyContinue
# FRDM-A = CAN sender (edge) on flexcan0 / J15, LPUART6 console (PuTTY).
west build -b s32k344mini .\firmware\k3_silicon -d build\frdma -p always -- `
  -DBOARD_ROOT=D:/zephyr-ws/Zephyr-Renode-S32K-sim `
  -DEXTRA_DTC_OVERLAY_FILE=frdma.overlay -DEXTRA_CONF_FILE=frdma.conf -DROLE_EDGE=1

Pause
