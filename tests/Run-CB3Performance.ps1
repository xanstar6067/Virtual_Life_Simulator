param([string]$World, [int]$Repeats = 3, [int[]]$Threads = @(1, 2, 4), [int]$DurationMs = 1000)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $repoRoot 'x64\perf-audit\baseline\cb3-perf.exe'
if (!(Test-Path -LiteralPath $exe)) { throw 'Run tests/Build-CB3Performance.ps1 first.' }
$outputDir = Join-Path $repoRoot 'x64\perf-audit\matrix'
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$cases = @()
foreach ($t in $Threads) {
    $cases += @{ Name = 'empty'; T = $t; Args = @('--mode', 'fixed', '--bots', '0') }
    foreach ($n in @(100000, 500000)) {
        $cases += @{ Name = "fixed-$n"; T = $t; Args = @('--mode', 'fixed', '--bots', "$n") }
    }
    $cases += @{ Name = 'duplicate-500000'; T = $t; Args = @('--mode', 'fixed', '--bots', '500000', '--duplicate-genomes') }
    $cases += @{ Name = 'clustered-100000'; T = $t; Args = @('--mode', 'fixed', '--bots', '100000', '--clustered') }
    $cases += @{ Name = 'skipbrain-500000'; T = $t; Args = @('--mode', 'skipbrain', '--bots', '500000') }
    $cases += @{ Name = 'stunned-500000'; T = $t; Args = @('--mode', 'stunned', '--bots', '500000') }
    if ($World) {
        $worldPath = (Resolve-Path -LiteralPath $World).Path
        $cases += @{ Name = 'snapshot-fixed'; T = $t; Args = @('--mode', 'fixed', '--world', $worldPath) }
        $cases += @{ Name = 'snapshot-skipbrain'; T = $t; Args = @('--mode', 'skipbrain', '--world', $worldPath) }
        $cases += @{ Name = 'snapshot-real'; T = $t; Args = @('--mode', 'real', '--world', $worldPath, '--warmup', '0', '--ticks', '25') }
    }
}
$rows = @()
Push-Location $repoRoot
try {
    # Repeat rounds and rotate case order to reduce systematic warmup/thermal bias.
    for ($r = 1; $r -le $Repeats; ++$r) {
        $ordered = if ($r % 2) { $cases } else { @($cases)[($cases.Count - 1)..0] }
        foreach ($case in $ordered) {
            $name = "$($case.Name)-$($case.T)t-r$r"
            $path = Join-Path $outputDir "$name.log"
            $arguments = @($case.Args) + @('--threads', "$($case.T)", '--ms', "$DurationMs")
            $lines = & $exe @arguments
            if ($LASTEXITCODE) { throw "Benchmark failed: $name" }
            $lines | Set-Content -LiteralPath $path -Encoding utf8
            $row = [ordered]@{ Case = $case.Name; Threads = $case.T; Repeat = $r }
            foreach ($prefix in @('RESULT', 'MEASURE_START', 'END', 'EXTRA')) {
                $line = $lines | Where-Object { $_.StartsWith("$prefix ") }
                if (!$line) { throw "Missing $prefix in $name" }
                foreach ($pair in ($line -split ' ' | Select-Object -Skip 1)) {
                    $key, $value = $pair -split '=', 2
                    $row["${prefix}_$key"] = [double]::Parse($value, [Globalization.CultureInfo]::InvariantCulture)
                }
            }
            $rows += [PSCustomObject]$row
            $rows | ForEach-Object {
                $portable = [ordered]@{}
                foreach ($property in $_.PSObject.Properties) {
                    $portable[$property.Name] = if ($property.Value -is [double]) {
                        $property.Value.ToString([Globalization.CultureInfo]::InvariantCulture)
                    } else { $property.Value }
                }
                [PSCustomObject]$portable
            } | Export-Csv -LiteralPath (Join-Path $outputDir 'results.csv') -NoTypeInformation -Encoding utf8
            Write-Output ("{0}: {1:N3} ms/tick, {2:N2} CPU cores" -f $name, $row.RESULT_ms_tick, $row.RESULT_cpu_cores)
        }
    }
} finally { Pop-Location }
