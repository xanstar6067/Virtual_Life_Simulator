param([int]$Repeats = 3,
      [string]$BeforeExecutable = 'x64/perf-audit/before-balancing/cb3-perf.exe',
      [string]$AfterExecutable = 'x64/perf-audit/baseline/cb3-perf.exe',
      [string]$CaseFilter = '.*',
      [string]$OutputDirectory = 'x64/perf-audit/balancing-comparison',
      [string]$ResultsCsv = 'CB3_BALANCING_RESULTS.csv')
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$outputDir = Join-Path $repoRoot $OutputDirectory
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
$executables = @{before = (Resolve-Path $BeforeExecutable).Path; after = (Resolve-Path $AfterExecutable).Path}
$cases = @(
    @{Name='cluster-100k'; T=1; Args=@('--mode','fixed','--bots','100000','--clustered','--warmup','32','--ticks','96')},
    @{Name='cluster-100k'; T=2; Args=@('--mode','fixed','--bots','100000','--clustered','--warmup','32','--ticks','96')},
    @{Name='cluster-100k'; T=4; Args=@('--mode','fixed','--bots','100000','--clustered','--warmup','32','--ticks','96')},
    @{Name='uniform-100k'; T=4; Args=@('--mode','fixed','--bots','100000','--warmup','32','--ticks','96')},
    @{Name='uniform-500k'; T=4; Args=@('--mode','fixed','--bots','500000','--warmup','32','--ticks','48')},
    @{Name='world-200k'; T=4; Args=@('--mode','real','--world','x64/perf-audit/checkpoints/growth-200000.world','--warmup','0','--ticks','25')},
    @{Name='world-500k'; T=4; Args=@('--mode','real','--world','x64/perf-audit/checkpoints/growth-500000.world','--warmup','0','--ticks','25')},
    @{Name='growth'; T=4; Args=@('--mode','growth','--waves','5','--ticks','500')}
)
$cases = @($cases | Where-Object { $_.Name -match $CaseFilter })
if (!$cases.Count -or $Repeats -lt 1) { throw 'Choose at least one case and one repeat.' }
function Metrics([string]$Line) {
    $m = [ordered]@{}
    foreach ($match in [regex]::Matches($Line, '(\w+)=([-+\d.eE]+)')) {
        $m[$match.Groups[1].Value] = [double]::Parse($match.Groups[2].Value, [Globalization.CultureInfo]::InvariantCulture)
    }
    return $m
}
function Export-Rows($rows) {
    $fields = @($rows | ForEach-Object { $_.PSObject.Properties.Name } | Select-Object -Unique)
    $rows | ForEach-Object {
        $row = [ordered]@{}
        foreach ($field in $fields) {
            $value = $_.$field
            $row[$field] = if ($value -is [double]) { $value.ToString([Globalization.CultureInfo]::InvariantCulture) } else { $value }
        }
        [pscustomobject]$row
    } | Export-Csv -LiteralPath (Join-Path $repoRoot $ResultsCsv) -NoTypeInformation -Encoding utf8
}
$rows = @()
Push-Location $repoRoot
try {
    for ($repeat = 1; $repeat -le $Repeats; ++$repeat) {
        $index = 0
        foreach ($case in $cases) {
            $variants = if (($repeat + $index++) % 2) { @('before','after') } else { @('after','before') }
            $pair = @{}
            $folders = @{}
            $pairRows = @()
            foreach ($variant in $variants) {
                $name = "$($case.Name)-$($case.T)t-$variant-r$repeat"
                $arguments = @($case.Args) + @('--threads', "$($case.T)")
                if ($case.Name -eq 'growth') {
                    $folders[$variant] = Join-Path $outputDir $name
                    $arguments += @('--out', $folders[$variant])
                }
                $lines = & $executables[$variant] @arguments
                if ($LASTEXITCODE) { throw "Benchmark failed: $name" }
                $lines | Set-Content -LiteralPath (Join-Path $outputDir "$name.log") -Encoding utf8
                $scheduler = @($lines | Where-Object { $_.StartsWith('SCHEDULER ') } | Select-Object -Last 1)
                $rebalances = if ($scheduler.Count) { (Metrics $scheduler[0])['rebalances'] } else { 0 }
                $observations = if ($case.Name -eq 'growth') { @($lines | Where-Object { $_.StartsWith('GROWTH ') }) } else { @($lines | Where-Object { $_.StartsWith('RESULT ') }) }
                $end = if ($case.Name -eq 'growth') { Metrics $observations[-1] } else { Metrics ($lines | Where-Object { $_.StartsWith('END ') }) }
                $pair[$variant] = $end
                foreach ($line in $observations) {
                    $row = [ordered]@{Case=$case.Name; Threads=$case.T; Variant=$variant; Repeat=$repeat; Rebalances=$rebalances}
                    foreach ($entry in (Metrics $line).GetEnumerator()) { $row[$entry.Key] = $entry.Value }
                    if ($case.Name -ne 'growth') {
                        foreach ($entry in $end.GetEnumerator()) { $row[$entry.Key] = $entry.Value }
                        $workers = @($lines | Where-Object { $_.StartsWith('WORKER ') } | ForEach-Object { Metrics $_ })
                        foreach ($key in @('objects0','objects1','phase0_ms','phase1_ms')) {
                            if ($workers.Count) { $row["max_$key"] = ($workers | ForEach-Object { $_[$key] } | Measure-Object -Maximum).Maximum }
                        }
                    }
                    $pairRows += [pscustomobject]$row
                }
                Write-Output "Completed $name (rebalances=$rebalances)"
            }
            $same = $true
            foreach ($key in @('bots','genomes','apples','organics','rocks')) {
                if ($pair.before[$key] -ne $pair.after[$key]) { $same = $false }
            }
            if ($case.Name -match '^(cluster|uniform)' -and !$same) { throw "Controlled population changed: $($case.Name)" }
            $checkpointMatches = ''
            if ($case.Name -eq 'growth') {
                $checkpointMatches = 0
                foreach ($threshold in @(50000,200000,500000)) {
                    $before = Join-Path $folders.before "growth-$threshold.world"
                    $after = Join-Path $folders.after "growth-$threshold.world"
                    if ((Test-Path $before) -and (Test-Path $after) -and
                        (Get-FileHash $before -Algorithm SHA256).Hash -eq (Get-FileHash $after -Algorithm SHA256).Hash) { ++$checkpointMatches }
                }
            }
            # Adaptive ordering intentionally permits different real trajectories.
            # Correctness is tested against serialized execution of the same layout.
            foreach ($row in $pairRows) {
                $row | Add-Member NoteProperty EndCountsEqual $same
                $row | Add-Member NoteProperty CheckpointsEqual $checkpointMatches
            }
            $rows += $pairRows
            Export-Rows $rows
            Write-Output "Pair counts equal=$same checkpoint matches=$checkpointMatches"
        }
    }
    foreach ($case in $cases) {
        foreach ($variant in @('before','after')) {
            $endpoints = @($rows | Where-Object {
                $_.Case -eq $case.Name -and $_.Threads -eq $case.T -and $_.Variant -eq $variant -and
                ($case.Name -ne 'growth' -or $_.frame -eq 500)
            })
            foreach ($key in @('bots','genomes','apples','organics','rocks','births','deaths')) {
                if (@($endpoints | ForEach-Object { $_.$key } | Select-Object -Unique).Count -ne 1) {
                    throw "Non-repeatable endpoint: $($case.Name), $variant, $key"
                }
            }
        }
    }
    if ($cases.Name -contains 'growth') {
        foreach ($variant in @('before','after')) {
            foreach ($threshold in @(50000,200000,500000)) {
                $hashes = @(1..$Repeats | ForEach-Object {
                    (Get-FileHash -LiteralPath (Join-Path $outputDir "growth-4t-$variant-r$_/growth-$threshold.world") -Algorithm SHA256).Hash
                } | Select-Object -Unique)
                if ($hashes.Count -ne 1) { throw "Non-repeatable checkpoint: $variant, $threshold" }
            }
        }
    }
    Write-Output 'Repeatability: endpoint counts and applicable within-version checkpoint hashes match.'
} finally { Pop-Location }
