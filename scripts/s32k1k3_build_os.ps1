echo "Build for K1 & K3 boards starting up..."
echo "[$env:ZEPHYR_BASE]"
echo "Checking for the path to the CMake zephyr_default configuraiton"
Test-Path D:\zephyr-ws\zephyr\cmake\modules\zephyr_default.cmake
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim
echo "Removing old configuration on K3 hub"
Remove-Item -Recurse -Force build\k3_hub -ErrorAction SilentlyContinue
echo "Generating .elf file for s32k344"
west build -b mr_canhubk3/s32k344 firmware\k3_hub -d build\k3_hub -p always

echo "Removing old configuration on K1 zonal"
Remove-Item -Recurse -Force build\k1_powertrain -ErrorAction SilentlyContinue
echo "Generating .elf file for s32k1"
west build -b mr_canhubk3/s32k344 firmware\k1_edge -d build\k1_powertrain 
pause

