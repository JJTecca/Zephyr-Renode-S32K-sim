Write-Output "Real-silicon build: 2x FRDM-A-S32K344 (both 16 MHz) -- edge sender + center receiver"
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim

$haldir = "D:\zephyr-ws\modules\hal\nxp\s32"

# Both boards are 16 MHz FRDM-A: restore stock clock config
git -C $haldir checkout -- soc/s32k344/src/Clock_Ip_Cfg.c

Write-Output "[1/2] Building elf for FRDM-A EDGE (sender) via /build folder"
Remove-Item -Recurse -Force build\frdma_edge -ErrorAction SilentlyContinue
west build -b s32k344mini firmware\k3_silicon -d build\frdma_edge -p always -- `
    -DBOARD_ROOT="D:/zephyr-ws/Zephyr-Renode-S32K-sim" -DNODE=frdma -DROLE_EDGE=1

Write-Output "[2/2] Building elf for FRDM-A CENTER (receiver) via /build folder"
Remove-Item -Recurse -Force build\frdma_center -ErrorAction SilentlyContinue
west build -b s32k344mini firmware\k3_silicon -d build\frdma_center -p always -- `
    -DBOARD_ROOT="D:/zephyr-ws/Zephyr-Renode-S32K-sim" -DNODE=frdma -DROLE_EDGE=0
Pause