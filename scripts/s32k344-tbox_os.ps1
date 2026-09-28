Write-Output "Real sillicon boards build: no Renode simulation, switch to the other ps1"
Write-Output "Build for S32K3-T-BOX (S32K344) starting up..."
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim
$board = "s32k344mini"

Remove-Item -Recurse -Force build\k3_tbox -ErrorAction SilentlyContinue
west build -b $board firmware\k3_hub -d build\k3_tbox -p always -- -DBOARD_ROOT=D:/zephyr-ws/Zephyr-Renode-S32K-sim
Pause