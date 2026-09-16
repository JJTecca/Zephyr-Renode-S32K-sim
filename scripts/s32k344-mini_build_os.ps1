Write-Output "Real sillicon boards build: no Renode simulation, switch to the other ps1"
Write-Output "Build for S32K344-mini EVB starting up..."
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim
Remove-Item -Recurse -Force build\k3_mini -ErrorAction SilentlyContinue
west build -b s32k344mini firmware\k3_hub -d build\k3_mini -p always -- -DBOARD_ROOT=D:/zephyr-ws/Zephyr-Renode-S32K-sim

Pause