# Run elevated (xperf PMC needs administrator rights). Hardware counters for the CB3 console harness (1 thread). Worlds are read-only; output in x64\perf-audit\pmc.
# Each case runs with a short and a long measurement; the difference removes loading/aging from per-tick numbers.
Set-Location (Split-Path -Parent $PSScriptRoot)
$xperf = 'C:\Program Files (x86)\Windows Kits\10\Windows Performance Toolkit\xperf.exe'
$exe = '.\x64\perf-audit\baseline\cb3-perf.exe'
$world = 'x64/perf-audit/checkpoints/growth-500000.world'
$out = 'x64\perf-audit\pmc'
New-Item -ItemType Directory -Force $out | Out-Null
$status = Join-Path $out 'status.txt'
"start $(Get-Date -Format o)" | Set-Content $status
$groups = [ordered]@{ ipc = 'InstructionRetired,TotalCycles'; dc = 'DcacheAccesses,DcacheMisses'; llc = 'CacheMisses,BranchMispredictions' }
$layouts = [ordered]@{ sequential = @('--bot-layout', 'sequential'); default = @(); shuffled = @('--bot-layout', 'shuffled'); aged = @('--age', '300') }
foreach ($layout in $layouts.Keys) {
    foreach ($mode in 'fixed', 'skipbrain') {
        foreach ($group in $groups.Keys) {
            foreach ($ms in 2000, 8000) {
                $base = Join-Path $out "$layout-$mode-$group-$ms"
                & $xperf -on PROC_THREAD+LOADER+CSWITCH -Pmc $groups[$group] CSWITCH 2>&1 | Out-Null
                & $exe --mode $mode --world $world --threads 1 --warmup 2 --ms $ms @($layouts[$layout]) *> "$base.log"
                & $xperf -d "$base.etl" 2>&1 | Out-Null
                & $xperf -i "$base.etl" -o "$base.txt" 2>&1 | Out-Null
                Remove-Item "$base.etl" -ErrorAction SilentlyContinue
                "done $layout $mode $group $ms" | Add-Content $status
            }
        }
    }
}
"finished $(Get-Date -Format o)" | Add-Content $status
