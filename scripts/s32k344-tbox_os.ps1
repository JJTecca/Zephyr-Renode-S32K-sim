Write-Output "Real-silicon build: FRDM-A (16 MHz sender) + T-box (40 MHz receiver)"
cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim

$hal    = "D:\zephyr-ws\modules\hal\nxp\s32\soc\s32k344\src\Clock_Ip_Cfg.c"
$haldir = "D:\zephyr-ws\modules\hal\nxp\s32"

# safety check clock config
git -C $haldir checkout -- soc/s32k344/src/Clock_Ip_Cfg.c

try {
    Write-Output "[1/2] Building elf for frdm-a via /build folder"
    Remove-Item -Recurse -Force build\frdma -ErrorAction SilentlyContinue
    west build -b s32k344mini firmware\k3_silicon -d build\frdma -p always -- `
        -DBOARD_ROOT="D:/zephyr-ws/Zephyr-Renode-S32K-sim" -DNODE=frdma -DROLE_EDGE=1

    Write-Output "[2/2] Building elf for t-box via /build folder"
    $c = Get-Content $hal -Raw
    $c = $c -replace '16000000U','40000000U' -replace '2U,(\s*/\* predivider \*/)','5U,$1'
    Set-Content $hal $c -NoNewline

    Remove-Item -Recurse -Force build\tbox -ErrorAction SilentlyContinue
    west build -b s32k344mini firmware\k3_silicon -d build\tbox -p always -- `
        -DBOARD_ROOT="D:/zephyr-ws/Zephyr-Renode-S32K-sim" -DNODE=tbox -DROLE_EDGE=0
}
finally {
    git -C $haldir checkout -- soc/s32k344/src/Clock_Ip_Cfg.c
}
Pause