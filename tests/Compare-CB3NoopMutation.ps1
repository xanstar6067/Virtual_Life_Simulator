param([int]$Repeats = 3,
      [string]$BeforeExecutable = 'x64/perf-audit/before-noop/cb3-perf.exe',
      [string]$AfterExecutable = 'x64/perf-audit/before-storage/cb3-perf.exe',
      [string]$OutputDirectory = 'x64/perf-audit/noop-comparison',
      [string]$ResultsCsv = 'CB3_NOOP_MUTATION_RESULTS.csv',
      [switch]$RequireSameGenomes)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$outputDir = Join-Path $repoRoot $OutputDirectory
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$executables = @{
    before = (Resolve-Path -LiteralPath $BeforeExecutable).Path
    after = (Resolve-Path -LiteralPath $AfterExecutable).Path
}
$cases = @(
    @{Name='growth'; T=4; Args=@('--mode','growth','--waves','5','--ticks','500')},
    @{Name='world-200000'; T=4; Args=@('--mode','real','--world','x64/perf-audit/checkpoints/growth-200000.world','--warmup','0','--ticks','25')},
    @{Name='world-500000'; T=1; Args=@('--mode','real','--world','x64/perf-audit/checkpoints/growth-500000.world','--warmup','0','--ticks','25')},
    @{Name='world-500000'; T=4; Args=@('--mode','real','--world','x64/perf-audit/checkpoints/growth-500000.world','--warmup','0','--ticks','25')}
)
function Parse-Metrics([string]$Line) {
    $metrics = [ordered]@{}
    foreach ($entry in ($Line -split ' ')) {
        if ($entry -notmatch '=') { continue }
        $key,$value = $entry -split '=',2
        $metrics[$key] = [double]::Parse($value,[Globalization.CultureInfo]::InvariantCulture)
    }
    return $metrics
}
$rows = @()
Push-Location $repoRoot
try {
    for ($r = 1; $r -le $Repeats; ++$r) {
        $index = 0
        foreach ($case in $cases) {
            $variants = if (($r + $index++) % 2) { @('before','after') } else { @('after','before') }
            $pair = @{}
            $checkpoints = @{}
            foreach ($variant in $variants) {
                $name = "$($case.Name)-$($case.T)t-$variant-r$r"
                $caseArguments = @($case.Args) + @('--threads', "$($case.T)")
                if ($case.Name -eq 'growth') {
                    $checkpoints[$variant] = Join-Path $outputDir $name
                    $caseArguments += @('--out',$checkpoints[$variant])
                }
                $lines = & $executables[$variant] @caseArguments
                if ($LASTEXITCODE) { throw "Benchmark failed: $name" }
                $lines | Set-Content -LiteralPath (Join-Path $outputDir "$name.log") -Encoding utf8
                if ($case.Name -eq 'growth') {
                    $observations = @($lines | Where-Object { $_.StartsWith('GROWTH ') })
                    if ($observations.Count -ne 20) { throw "Incomplete growth: $name" }
                    $pair[$variant] = @($observations | ForEach-Object { Parse-Metrics $_ })
                    foreach ($line in $observations) {
                        $row = [ordered]@{Case=$case.Name; Threads=$case.T; Variant=$variant; Repeat=$r}
                        foreach ($entry in (Parse-Metrics $line).GetEnumerator()) { $row[$entry.Key] = $entry.Value }
                        $rows += [PSCustomObject]$row
                    }
                } else {
                    $row = [ordered]@{Case=$case.Name; Threads=$case.T; Variant=$variant; Repeat=$r; frame=0}
                    foreach ($prefix in @('START','RESULT','END')) {
                        $line = @($lines | Where-Object { $_.StartsWith("$prefix ") })
                        if ($line.Count -ne 1) { throw "Missing or duplicate $prefix in $name" }
                        foreach ($entry in (Parse-Metrics $line[0]).GetEnumerator()) {
                            $row["${prefix}_$($entry.Key)"] = $entry.Value
                        }
                    }
                    $pair[$variant] = $row
                    $rows += [PSCustomObject]$row
                }
                # Use the union of fields so growth and replay metrics both survive CSV export.
                $fields = @($rows | ForEach-Object { $_.PSObject.Properties.Name } | Select-Object -Unique)
                $rows | ForEach-Object {
                    $portable = [ordered]@{}
                    foreach ($field in $fields) {
                        $value = $_.$field
                        $portable[$field] = if ($value -is [double]) {
                            $value.ToString([Globalization.CultureInfo]::InvariantCulture)
                        } else { $value }
                    }
                    [PSCustomObject]$portable
                } | Export-Csv -LiteralPath (Join-Path $repoRoot $ResultsCsv) -NoTypeInformation -Encoding utf8
                Write-Output "Completed $name"
            }
            if ($case.Name -eq 'growth') {
                for ($i = 0; $i -lt $pair.before.Count; ++$i) {
                    foreach ($key in @('frame','bots','apples','organics','rocks','births','deaths')) {
                        if ($pair.before[$i][$key] -ne $pair.after[$i][$key]) { throw "Growth state differs: frame $i, $key" }
                    }
                    if ($RequireSameGenomes -and $pair.before[$i]['genomes'] -ne $pair.after[$i]['genomes']) {
                        throw "Growth genome counts differ: frame $i"
                    }
                }
                foreach ($threshold in @(50000,200000,500000)) {
                    $file = "growth-$threshold.world"
                    $beforeHash = (Get-FileHash -LiteralPath (Join-Path $checkpoints.before $file) -Algorithm SHA256).Hash
                    $afterHash = (Get-FileHash -LiteralPath (Join-Path $checkpoints.after $file) -Algorithm SHA256).Hash
                    if ($beforeHash -ne $afterHash) { throw "Saved world differs: $file" }
                    Write-Output "Identical SHA-256: $file, repeat $r"
                }
            } else {
                foreach ($key in @('START_bots','START_genomes','RESULT_births','RESULT_deaths','END_bots','END_apples','END_organics','END_rocks')) {
                    if ($pair.before[$key] -ne $pair.after[$key]) { throw "Replay state differs: $($case.Name), $key" }
                }
                if ($RequireSameGenomes -and $pair.before['END_genomes'] -ne $pair.after['END_genomes']) {
                    throw "Replay genome counts differ: $($case.Name)"
                }
            }
        }
    }
} finally { Pop-Location }
