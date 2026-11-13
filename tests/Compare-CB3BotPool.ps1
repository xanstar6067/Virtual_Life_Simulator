# Paired comparison of the current CB3 harness with an archived one (by default the build before the bot pool).
# Needs x64/perf-audit/<Before>/cb3-perf.exe (+ SDL2.dll), the current harness from Build-CB3Performance.ps1
# and growth-500000.world from the growth example in tests/README.md. Runs sequentially, alternating the order.
# -Before before-brain -OutputDirectory x64\perf-audit\brain-comparison -ResultsCsv CB3_BRAIN_SKIP_RESULTS.csv
# reproduces the comparison of the zero-skipping brain plan.
param([int]$Repeats = 3, [string]$Before = 'before-pool', [string]$OutputDirectory = 'x64\perf-audit\pool-comparison',
    [string]$ResultsCsv = 'CB3_BOT_POOL_RESULTS.csv')
[Threading.Thread]::CurrentThread.CurrentCulture = [Globalization.CultureInfo]::InvariantCulture
Set-Location (Split-Path -Parent $PSScriptRoot)
foreach ($required in "x64\perf-audit\$Before\cb3-perf.exe", 'x64\perf-audit\baseline\cb3-perf.exe', 'x64\perf-audit\checkpoints\growth-500000.world') {
    if (!(Test-Path -LiteralPath $required)) { throw "Missing $required" }
}
$world = 'x64/perf-audit/checkpoints/growth-500000.world'
$logDir = $OutputDirectory
New-Item -ItemType Directory -Force $logDir | Out-Null
$cases = [ordered]@{
    'real'            = @('--mode', 'real', '--world', $world, '--warmup', '0', '--ticks', '25')
    'fixed'           = @('--mode', 'fixed', '--world', $world, '--warmup', '2', '--ms', '3000')
    'skipbrain'       = @('--mode', 'skipbrain', '--world', $world, '--warmup', '2', '--ms', '3000')
    'aged-real'       = @('--mode', 'real', '--world', $world, '--warmup', '300', '--ticks', '25')
    'aged-fixed'      = @('--mode', 'fixed', '--world', $world, '--age', '300', '--warmup', '2', '--ms', '3000')
    'aged-skipbrain'  = @('--mode', 'skipbrain', '--world', $world, '--age', '300', '--warmup', '2', '--ms', '3000')
}
$rows = @()
for ($r = 1; $r -le $Repeats; ++$r) {
    foreach ($threads in 1, 16) {
        foreach ($case in $cases.Keys) {
            $versions = @($Before, 'baseline'); if ($r % 2 -eq 0) { [array]::Reverse($versions) }
            foreach ($v in $versions) {
                $log = Join-Path $logDir "$v-$case-t$threads-r$r.log"
                & ".\x64\perf-audit\$v\cb3-perf.exe" @($cases[$case]) --threads $threads *> $log
                $text = Get-Content $log
                $res = ($text | Select-String '^RESULT' | Select-Object -Last 1).Line
                $ms0 = ($text | Select-String '^MEASURE_START' | Select-Object -Last 1).Line
                $end = ($text | Select-String '^END' | Select-Object -Last 1).Line
                if (!$res) { Write-Output "FAILED $log"; continue }
                $get = { param($line, $key) [double]([regex]::Match($line, "(?:^| )$key=([\d.]+)").Groups[1].Value) }
                $ms = & $get $res 'ms_tick'; $cores = & $get $res 'cpu_cores'; $bots = & $get $ms0 'bots'
                $rows += [pscustomobject]@{ version = $(if ($v -eq "baseline") { "after" } else { "before" }); case = $case; threads = $threads; repeat = $r; bots = $bots;
                    ms_tick = $ms; cpu_ns_per_bot = [math]::Round($ms * $cores * 1e6 / $bots, 1);
                    private_mib = (& $get $end 'private_mib'); births = (& $get $res 'births') }
                Write-Output ("{0} {1} t{2} r{3}: bots={4} {5:N2} ms, {6:N1} ns/bot, private {7} MiB" -f $v, $case, $threads, $r, $bots, $ms, ($ms * $cores * 1e6 / $bots), (& $get $end 'private_mib'))
            }
        }
    }
}
$rows | Export-Csv -NoTypeInformation -Encoding utf8 $ResultsCsv
