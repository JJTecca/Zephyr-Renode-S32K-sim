cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim
./scripts/s32k344-tbox_os.ps1

$Fault      = "memory_leak"
$Gdb        = "D:\S32DS\S32DS\tools\gdb-arm\arm32-eabi\bin\arm-none-eabi-gdb.exe"
$PeServer   = "D:\S32DS\eclipse\plugins\com.pemicro.debug.gdbjtag.pne_6.2.4.202609141407\win32\pegdbserver_console.exe"
$PeDir      = Split-Path $PeServer
$Plink      = "C:\Program Files\PuTTY\plink.exe"
$EdgePort   = "USB1"
$CenterPort = "USB2"
$EdgeSrv    = "-device=NXP_S32K3xx_S32K344 -startserver -singlesession -serverport=7224 -gdbmiport=6224 -interface=USBMULTILINK -speed=5000 -port=$EdgePort -usecyclonerelays=1 -currentalgorithmindex=0 -streamingport=10224 -javalinkport=59518"
$CenterSrv  = "-device=NXP_S32K3xx_S32K344 -startserver -serverport=7324 -gdbmiport=6324 -interface=USBMULTILINK -speed=5000 -port=$CenterPort -usecyclonerelays=1 -currentalgorithmindex=0 -streamingport=10324 -javalinkport=59519"
$EdgeElf    = "build\frdma\zephyr\zephyr.elf"
$CenterElf  = "build\tbox\zephyr\zephyr.elf"
$ComEdge    = "COM4"
$stem       = "$($Fault)_256_seed1"
New-Item -ItemType Directory -Force -Path evidence\silicon | Out-Null

$srvC = Start-Process $PeServer -ArgumentList $CenterSrv -PassThru -WorkingDirectory $PeDir -RedirectStandardOutput "evidence\silicon\${stem}_center.log"
Start-Sleep 3
$gdbC = Start-Process $Gdb -ArgumentList "$CenterElf -batch -x scripts\silicon\run_center.gdb" -PassThru -NoNewWindow
Start-Sleep 10

$srvE = Start-Process $PeServer -ArgumentList $EdgeSrv -PassThru -WorkingDirectory $PeDir
Start-Sleep 3
& $Gdb $EdgeElf -batch -ex "set tcp auto-retry on" -ex "set tcp connect-timeout 240" -ex "target remote :7224" -ex "load" -ex "monitor reset" -ex "detach" -ex "quit"
Stop-Process $srvE -ErrorAction SilentlyContinue

$pE = Start-Process $Plink -ArgumentList "-serial $ComEdge -sercfg 115200,8,n,1,N" -RedirectStandardOutput "evidence\silicon\${stem}_edge.log" -PassThru

Start-Sleep -Seconds 30

$gdbFile = if ($Fault -eq "memory_leak") { "scripts\silicon\inject_leak.gdb" } else { "scripts\silicon\inject_busy.gdb" }
$srv = Start-Process $PeServer -ArgumentList $EdgeSrv -PassThru -WorkingDirectory $PeDir
Start-Sleep 3
& $Gdb $EdgeElf -batch -x $gdbFile
Stop-Process $srv -ErrorAction SilentlyContinue

Start-Sleep -Seconds 30
Stop-Process $pE -ErrorAction SilentlyContinue
Stop-Process $gdbC -ErrorAction SilentlyContinue
Stop-Process $srvC -ErrorAction SilentlyContinue

if ($Fault -eq "memory_leak") {
    python sim\run_campaign.py --log evidence\silicon\${stem}_edge.log --fault $Fault --rate 256 --outdir datasets/silicon
} else {
    python sim\run_campaign.py --log evidence\silicon\${stem}_edge.log --fault $Fault --outdir datasets/silicon
}
python ml\dataset.py   --glob "datasets/silicon/$($Fault)_*.csv"
python ml\baseline.py  --glob "datasets/silicon/$($Fault)_*.csv"
if ($Fault -eq "memory_leak") { python ml\predictor.py --glob "datasets/silicon/memory_leak_*.csv" }
python ml\metrics.py --campaign evidence\silicon
python ml\train_ae.py  --glob "datasets/silicon/$($Fault)_*.csv"
python ml\quantize.py
python ml\export_model.py
Pause