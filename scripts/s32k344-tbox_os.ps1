Write-Output "Real-silicon build: 2x FRDM-A edges (16 MHz) + T-box hub (40 MHz)"
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim

$hal    = "D:\zephyr-ws\modules\hal\nxp\s32\soc\s32k344\src\Clock_Ip_Cfg.c"
$haldir = "D:\zephyr-ws\modules\hal\nxp\s32"
git -C $haldir checkout -- soc/s32k344/src/Clock_Ip_Cfg.c

try {
    Write-Output "[1/3] edge1 FRDM-A (16 MHz, node=1 POWERTRAIN) -> build\frdma1"
    Remove-Item -Recurse -Force build\frdma1 -ErrorAction SilentlyContinue
    west build -b s32k344mini firmware\k3_silicon -d build\frdma1 -p always -- `
        -DBOARD_ROOT="D:/zephyr-ws/Zephyr-Renode-S32K-sim" -DNODE=frdma -DROLE_EDGE=1 -DSDV_NODE=1
    if ($LASTEXITCODE -ne 0) { throw "edge1 build failed ($LASTEXITCODE)" }

    Write-Output "[2/3] edge2 FRDM-A (16 MHz, node=2 CHASSIS) -> build\frdma2"
    Remove-Item -Recurse -Force build\frdma2 -ErrorAction SilentlyContinue
    west build -b s32k344mini firmware\k3_silicon -d build\frdma2 -p always -- `
        -DBOARD_ROOT="D:/zephyr-ws/Zephyr-Renode-S32K-sim" -DNODE=frdma -DROLE_EDGE=1 -DSDV_NODE=2
    if ($LASTEXITCODE -ne 0) { throw "edge2 build failed ($LASTEXITCODE)" }

    Write-Output "[3/3] T-box hub (40 MHz, center) -> build\tbox"
    $c = Get-Content $hal -Raw
    $c = $c -replace '16000000U','40000000U' -replace '2U,(\s*/\* predivider \*/)','5U,$1'
    Set-Content $hal $c -NoNewline
    Remove-Item -Recurse -Force build\tbox -ErrorAction SilentlyContinue
    west build -b s32k344mini firmware\k3_silicon -d build\tbox -p always -- `
        -DBOARD_ROOT="D:/zephyr-ws/Zephyr-Renode-S32K-sim" -DNODE=tbox -DROLE_EDGE=0
    if ($LASTEXITCODE -ne 0) { throw "tbox build failed ($LASTEXITCODE)" }
}
finally {
    git -C $haldir checkout -- soc/s32k344/src/Clock_Ip_Cfg.c
}
Pause