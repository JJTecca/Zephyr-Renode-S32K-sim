cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim

./scripts/s32k344-tbox_os.ps1

$Fault      = "memory_leak"
$Gdb        = "D:\S32DS\S32DS\tools\gdb-arm\arm32-eabi\bin\arm-none-eabi-gdb.exe"
$PeServer   = "D:\S32DS\eclipse\plugins\com.pemicro.debug.gdbjtag.pne_6.2.4.202609141407\win32\pegdbserver_console.exe"
$PeDir      = Split-Path $PeServer
$Putty      = "C:\Program Files\PuTTY\putty.exe"
$Edge1Port  = "USB1"
$Edge2Port  = "USB2"
$CenterPort = "USB3"
$Edge1Srv   = "-device=NXP_S32K3xx_S32K344 -startserver -singlesession -serverport=7224 -gdbmiport=6224 -interface=USBMULTILINK -speed=5000 -port=$Edge1Port -usecyclonerelays=1 -currentalgorithmindex=0 -streamingport=10224 -javalinkport=59518"
$Edge2Srv   = "-device=NXP_S32K3xx_S32K344 -startserver -singlesession -serverport=7244 -gdbmiport=6244 -interface=USBMULTILINK -speed=5000 -port=$Edge2Port -usecyclonerelays=1 -currentalgorithmindex=0 -streamingport=10244 -javalinkport=59520"
$CenterSrv  = "-device=NXP_S32K3xx_S32K344 -startserver -singlesession -serverport=7324 -gdbmiport=6324 -interface=USBMULTILINK -speed=5000 -port=$CenterPort -usecyclonerelays=1 -currentalgorithmindex=0 -streamingport=10324 -javalinkport=59519"
$Edge1Elf   = "build\frdma1\zephyr\zephyr.elf"
$Edge2Elf   = "build\frdma2\zephyr\zephyr.elf"
$CenterElf  = "build\tbox\zephyr\zephyr.elf"
$ComEdge1   = "COM4"
$ComEdge2   = "COM7"
$stem       = "$($Fault)_256"
New-Item -ItemType Directory -Force -Path evidence\silicon | Out-Null

$srvC = Start-Process $PeServer -ArgumentList $CenterSrv -PassThru -WorkingDirectory $PeDir
Start-Sleep 3
& $Gdb $CenterElf -batch -ex "set tcp auto-retry on" -ex "set tcp connect-timeout 240" -ex "target remote :7324" -ex "load" -ex "monitor reset" -ex "detach" -ex "quit"
Stop-Process $srvC -ErrorAction SilentlyContinue

$srv1 = Start-Process $PeServer -ArgumentList $Edge1Srv -PassThru -WorkingDirectory $PeDir
Start-Sleep 3
& $Gdb $Edge1Elf -batch -ex "set tcp auto-retry on" -ex "set tcp connect-timeout 240" -ex "target remote :7224" -ex "load" -ex "monitor reset" -ex "detach" -ex "quit"
Stop-Process $srv1 -ErrorAction SilentlyContinue

$srv2 = Start-Process $PeServer -ArgumentList $Edge2Srv -PassThru -WorkingDirectory $PeDir
Start-Sleep 3
& $Gdb $Edge2Elf -batch -ex "set tcp auto-retry on" -ex "set tcp connect-timeout 240" -ex "target remote :7244" -ex "load" -ex "monitor reset" -ex "detach" -ex "quit"
Stop-Process $srv2 -ErrorAction SilentlyContinue

Get-Process putty,plink -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
$pE1 = Start-Process $Putty -ArgumentList "-serial $ComEdge1 -sercfg 115200,8,n,1,N -sessionlog evidence\silicon\${stem}_edge1.log" -PassThru
$pE2 = Start-Process $Putty -ArgumentList "-serial $ComEdge2 -sercfg 115200,8,n,1,N -sessionlog evidence\silicon\${stem}_edge2.log" -PassThru

Start-Sleep -Seconds 65

Stop-Process $pE1,$pE2 -ErrorAction SilentlyContinue

if ($Fault -eq "memory_leak") {
    python sim\run_campaign.py --log evidence\silicon\${stem}_edge1.log --fault $Fault --rate 256 --seed 1 --outdir datasets/silicon
    python sim\run_campaign.py --log evidence\silicon\${stem}_edge2.log --fault $Fault --rate 256 --seed 2 --outdir datasets/silicon
} else {
    python sim\run_campaign.py --log evidence\silicon\${stem}_edge1.log --fault $Fault --seed 1 --outdir datasets/silicon
    python sim\run_campaign.py --log evidence\silicon\${stem}_edge2.log --fault $Fault --seed 2 --outdir datasets/silicon
}
python ml\dataset.py   --glob "datasets/silicon/$($Fault)_*.csv"
python ml\baseline.py  --glob "datasets/silicon/$($Fault)_*.csv"
if ($Fault -eq "memory_leak") { python ml\predictor.py --glob "datasets/silicon/memory_leak_*.csv" }
python ml\metrics.py --campaign evidence\silicon
python ml\train_ae.py  --glob "datasets/silicon/$($Fault)_*.csv"
python ml\quantize.py
python ml\export_model.py
Pause