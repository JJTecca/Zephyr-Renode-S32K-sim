# run_all_ci.ps1 -- headless CI-style twin of run_all.ps1.
# Instead of opening Renode and pasting by hand, it drives the k1_k3_uart.robot
# test with renode-test, looping every leak rate x seed (+ deadline_miss), and
# generates/overwrites the labelled CSVs in /datasets. No human in the loop.

param(
  [int[]]$Rates       = @(64, 128, 256),
  [int]$Seeds         = 3,
  [string[]]$Classes  = @("memory_leak", "deadline_miss"),
  # renode-test launcher; set the full path if it is not on PATH
  # (e.g. "C:\Program Files\Renode\renode-test.bat")
  [string]$RenodeTest = "renode-test"
)

$repo  = "D:\zephyr-ws\Zephyr-Renode-S32K-sim"
$log   = "$repo\k1_telem.log"
$robot = "$repo\tests\renode\k1_k3_uart.robot"

Write-Output "Building firmware once (K1 + K3)..."
& "$repo\scripts\s32k1k3_build_os.ps1"
if ($LASTEXITCODE -ne 0) { throw "build failed" }

foreach ($cls in $Classes) {
    # deadline_miss has no rate axis -> single pass
    $rateList = if ($cls -eq "memory_leak") { $Rates } else { @(0) }

    foreach ($rate in $rateList) {
        for ($seed = 1; $seed -le $Seeds; $seed++) {
            Remove-Item $log -ErrorAction SilentlyContinue      # fresh log per run

            $env:CAMPAIGN_LOG   = $log
            $env:CAMPAIGN_ONSET = (1 + 2 * ($seed - 1)).ToString()   # seed -> different onset -> distinct episode
            $env:CAMPAIGN_DRAIN = "12"
            $env:CAMPAIGN_INJECT = if ($cls -eq "memory_leak") { "inject_memory_leak k1_powertrain $rate" }
                                   else                          { "inject_busy_spin  k1_powertrain 20000" }

            Write-Output "== $cls  rate=$rate  seed=$seed  onset=$($env:CAMPAIGN_ONSET)s =="
            & $RenodeTest $robot

            if (-not (Test-Path $log) -or -not (Select-String -Path $log -Pattern '^TELEM,' -Quiet)) {
                Write-Warning "No TELEM captured for $cls/$rate/$seed -- skipping"
                continue
            }

            if ($cls -eq "memory_leak") {
                python "$repo\sim\run_campaign.py" --log $log --fault $cls --rate $rate --seed $seed
            } else {
                python "$repo\sim\run_campaign.py" --log $log --fault $cls --seed $seed
            }
        }
    }
}

Write-Output "`nDatasets in /datasets:"
Get-ChildItem "$repo\datasets\*.csv" | Select-Object Name

Write-Output "`nML over the campaign:"
python "$repo\ml\dataset.py"   --glob "datasets/memory_leak_*.csv"
python "$repo\ml\baseline.py"  --glob "datasets/memory_leak_*.csv"
python "$repo\ml\predictor.py" --glob "datasets/memory_leak_*.csv"
