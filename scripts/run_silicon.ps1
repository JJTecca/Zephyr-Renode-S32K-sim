cd D:\zephyr-ws
.\.venv\Scripts\Activate.ps1
west zephyr-export
cd Zephyr-Renode-S32K-sim

# Script vars
$Fault      = "memory_leak"   # or "deadline_miss"
$Gdb        = "D:\S32DS\S32DS\tools\gdb-arm\arm32-eabi\bin\arm-none-eabi-gdb.exe"
$PeServer   = "D:\S32DS\eclipse\plugins\com.pemicro.debug.gdbjtag.pne_6.2.4.202609141407\win32\pegdbserver_console.exe"
$PeDir      = Split-Path $PeServer
$EdgePort   = "USB1"   # swap with CenterPort if COM7 shows role=center
$CenterPort = "USB2"
$EdgeSrv    = "-device=NXP_S32K3xx_S32K344 -startserver -singlesession -serverport=7224 -gdbmiport=6224 -interface=USBMULTILINK -speed=5000 -port=$EdgePort -usecyclonerelays=1 -currentalgorithmindex=0 -streamingport=10224 -javalinkport=59518"
$CenterSrv  = "-device=NXP_S32K3xx_S32K344 -startserver -singlesession -serverport=7324 -gdbmiport=6324 -interface=USBMULTILINK -speed=5000 -port=$CenterPort -usecyclonerelays=1 -currentalgorithmindex=0 -streamingport=10324 -javalinkport=59519"
$EdgeElf    = "build\frdma_edge\zephyr\zephyr.elf"
$CenterElf  = "build\frdma_center\zephyr\zephyr.elf"
$ComEdge    = "COM4"   # sender   (edge console; verify via 2BOARD,boot,role=edge)
$ComCenter  = "COM7"   # receiver (center console)
$Putty      = "C:\Program Files\PuTTY\putty.exe"
$stem       = "$($Fault)_256_seed1"
New-Item -ItemType Directory -Force -Path evidence\silicon | Out-Null

Write-Output "S32K344 FRDM-A Local CI Debugging Workflow"
Write-Output "Steps: "
"[1/7] Compile .c k3_silicon/ into elf binary"
"[2/7] Open up S32DS Platform Workspace silent console"
"[3/7] Setup the PuTTY consoles on COM4+COM7 ports with 115200 speed"
"[4/7] Write firmware on FRDM-A S32K344 via 7224 7324 server ports"
"[5/7] Wait for 30s telemetry clean send"
"[6/7] Inject memory_leak or deadline_miss into the boards"
"[7/7] Watch what happens save the logs & rest is like Renode"
./scripts/s32k344-2xfrdm-a.ps1

# 0. flash both boards headless (pegdbserver + gdb load); -singlesession auto-exits each
$srvC = Start-Process $PeServer -ArgumentList $CenterSrv -PassThru -WorkingDirectory $PeDir
Start-Sleep 3
& $Gdb $CenterElf -batch `
  -ex "set tcp auto-retry on" -ex "set tcp connect-timeout 240" `
  -ex "target remote :7324" -ex "load" -ex "monitor reset" -ex "detach" -ex "quit"
Stop-Process $srvC -ErrorAction SilentlyContinue

$srvE = Start-Process $PeServer -ArgumentList $EdgeSrv -PassThru -WorkingDirectory $PeDir
Start-Sleep 3
& $Gdb $EdgeElf -batch `
  -ex "set tcp auto-retry on" -ex "set tcp connect-timeout 240" `
  -ex "target remote :7224" -ex "load" -ex "monitor reset" -ex "detach" -ex "quit"
Stop-Process $srvE -ErrorAction SilentlyContinue

# 1. open both PuTTY consoles WITH logging (you watch them live + they capture)
$pE = Start-Process $Putty "-serial $ComEdge   -sercfg 115200,8,n,1,N -sessionlog evidence\silicon\${stem}_edge.log"   -PassThru
$pC = Start-Process $Putty "-serial $ComCenter -sercfg 115200,8,n,1,N -sessionlog evidence\silicon\${stem}_center.log" -PassThru

# 2. 30s of clean baseline telemetry
Start-Sleep -Seconds 30

# 3. inject on the edge via our own one-shot PEmicro server (single-session safe)
$gdbFile = if ($Fault -eq "memory_leak") { "scripts\silicon\inject_leak.gdb" } else { "scripts\silicon\inject_busy.gdb" }
$srv = Start-Process $PeServer -ArgumentList $EdgeSrv -PassThru -WorkingDirectory $PeDir
Start-Sleep 3
& $Gdb $EdgeElf -batch -x $gdbFile
Stop-Process $srv -ErrorAction SilentlyContinue

# 4. fault window, then auto-close the consoles
Start-Sleep -Seconds 30
Stop-Process $pE,$pC -ErrorAction SilentlyContinue

# 5. same ML pipeline as Renode, on the silicon logs (identical TELEM schema)
if ($Fault -eq "memory_leak") {
    python sim\run_campaign.py --log evidence\silicon\${stem}_edge.log --fault $Fault --rate 256 --outdir datasets/silicon
} else {
    python sim\run_campaign.py --log evidence\silicon\${stem}_edge.log --fault $Fault --outdir datasets/silicon
}
python ml\dataset.py   --glob "datasets/silicon/$($Fault)_*.csv"
python ml\baseline.py  --glob "datasets/silicon/$($Fault)_*.csv"
if ($Fault -eq "memory_leak") { python ml\predictor.py --glob "datasets/silicon/memory_leak_*.csv" }
python ml\metrics.py --campaign evidence\silicon
Pause