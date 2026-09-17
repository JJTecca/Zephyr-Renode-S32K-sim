Write-Output "Real silicon board build: no Renode simulation, switch to the other ps1"
Write-Output "Build for S32K144EVB-Q100 starting up..."
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim
Remove-Item -Recurse -Force build\k1_q100 -ErrorAction SilentlyContinue
west build -b s32k144evb firmware\k1_edge -d build\k1_q100 -p always -- `
  -DBOARD_ROOT=D:/zephyr-ws/Zephyr-Renode-S32K-sim -DCONFIG_SDV_NODE_ID=1

Pause
