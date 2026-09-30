cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim

$hal    = "D:\zephyr-ws\modules\hal\nxp\s32\soc\s32k344\src\Clock_Ip_Cfg.c"
$haldir = "D:\zephyr-ws\modules\hal\nxp\s32"
try {
    # 40 MHz T-BOX is a MUST since the FRDM-A uses a 16Mhz crystal
    $c = Get-Content $hal -Raw
    $c = $c -replace '16000000U','40000000U' -replace '2U,(\s*/\* predivider \*/)','5U,$1'
    Set-Content $hal $c -NoNewline

    west build -b s32k344mini firmware\can_loopback_test -d build\k3_loopback_tbox -p always -- -DBOARD_ROOT="D:/zephyr-ws/Zephyr-Renode-S32K-sim"
}
finally {
    git -C $haldir checkout -- soc/s32k344/src/Clock_Ip_Cfg.c   # restore pristine vendor file
}
Pause