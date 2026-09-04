Write-Output "Build for K1 & K3 boards starting up..."
Write-Output "[$env:ZEPHYR_BASE]"
Write-Output "Checking for the path to the CMake zephyr_default configuraiton"
Test-Path D:\zephyr-ws\zephyr\cmake\modules\zephyr_default.cmake
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim
Write-Output "Removing old configuration on K3 hub"
Remove-Item -Recurse -Force build\k3_hub -ErrorAction SilentlyContinue
Write-Output "Generating .elf file for s32k344"
west build -b mr_canhubk3/s32k344 firmware\k3_hub -d build\k3_hub -p always

Write-Output "Removing old configuration on K1 nodes"
Remove-Item -Recurse -Force build\k1_powertrain -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force build\k1_chassis    -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force build\k1_body       -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force build\k1_acoustic   -ErrorAction SilentlyContinue
Write-Output "Generating 4x .elf file for s32k1"
west build -b mr_canhubk3/s32k344 firmware\k1_edge -d build\k1_powertrain -p always -- -DCONFIG_SDV_NODE_ID=1
west build -b mr_canhubk3/s32k344 firmware\k1_edge -d build\k1_chassis    -p always -- -DCONFIG_SDV_NODE_ID=2
west build -b mr_canhubk3/s32k344 firmware\k1_edge -d build\k1_body       -p always -- -DCONFIG_SDV_NODE_ID=3
west build -b mr_canhubk3/s32k344 firmware\k1_edge -d build\k1_acoustic   -p always -- -DCONFIG_SDV_NODE_ID=4
pause