param([int]$Repeats = 3,
      [string]$BeforeExecutable = 'x64/perf-audit/before-plan/cb3-perf.exe',
      [string]$AfterExecutable = 'x64/perf-audit/baseline/cb3-perf.exe')
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$outputDir = Join-Path $repoRoot 'x64\perf-audit\plan-comparison'
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$executables = @{
    before = (Resolve-Path -LiteralPath $BeforeExecutable).Path
    after = (Resolve-Path -LiteralPath $AfterExecutable).Path
}
$cases = @()
foreach ($threshold in @(50000, 200000)) {
    $cases += @{Name="world-$threshold-real"; T=4; Args=@('--mode','real','--world',"x64/perf-audit/checkpoints/growth-$threshold.world",'--warmup','0','--ticks','25')}
}
foreach ($t in @(1, 2, 4)) {
    $cases += @{Name='world-500000-real'; T=$t; Args=@('--mode','real','--world','x64/perf-audit/checkpoints/growth-500000.world','--warmup','0','--ticks','25')}
}
foreach ($t in @(1, 4)) {
    $cases += @{Name='world-500000-fixed'; T=$t; Args=@('--mode','fixed','--world','x64/perf-audit/checkpoints/growth-500000.world','--warmup','0','--ticks','25')}
}
$cases += @{Name='synthetic-500000-fixed'; T=4; Args=@('--mode','fixed','--bots','500000','--pool','1024','--ms','1500')}
$rows = @()
Push-Location $repoRoot
try {
    for ($r = 1; $r -le $Repeats; ++$r) {
        $index = 0
        foreach ($case in $cases) {
            $variants = if (($r + $index++) % 2) { @('before','after') } else { @('after','before') }
            $pair = @{}
            foreach ($variant in $variants) {
                $name = "$($case.Name)-$($case.T)t-$variant-r$r"
                $caseArguments = @($case.Args) + @('--threads', "$($case.T)")
                $lines = & $executables[$variant] @caseArguments
                if ($LASTEXITCODE) { throw "Benchmark failed: $name" }
                $lines | Set-Content -LiteralPath (Join-Path $outputDir "$name.log") -Encoding utf8
                $row = [ordered]@{Case=$case.Name; Threads=$case.T; Variant=$variant; Repeat=$r}
                foreach ($prefix in @('START','RESULT','END','EXTRA')) {
                    $line = $lines | Where-Object { $_.StartsWith("$prefix ") }
                    if (!$line) { throw "Missing $prefix in $name" }
                    foreach ($entry in ($line -split ' ' | Select-Object -Skip 1)) {
                        $key,$value = $entry -split '=',2
                        $row["${prefix}_$key"] = [double]::Parse($value,[Globalization.CultureInfo]::InvariantCulture)
                    }
                }
                if (!$row.Contains('START_plan_mib')) { $row['START_plan_mib'] = 0.0; $row['END_plan_mib'] = 0.0 }
                $pair[$variant] = $row
                $rows += [PSCustomObject]$row
                $rows | ForEach-Object {
                    $portable = [ordered]@{}
                    foreach ($property in $_.PSObject.Properties) {
                        $portable[$property.Name] = if ($property.Value -is [double]) {
                            $property.Value.ToString([Globalization.CultureInfo]::InvariantCulture)
                        } else { $property.Value }
                    }
                    [PSCustomObject]$portable
                } | Export-Csv -LiteralPath (Join-Path $repoRoot 'CB3_BRAIN_PLAN_RESULTS.csv') -NoTypeInformation -Encoding utf8
                Write-Output ("{0}: {1:N3} ms/tick, {2:N1} MiB" -f $name,$row.RESULT_ms_tick,$row.START_private_mib)
            }
            foreach ($key in @('START_bots','START_genomes','RESULT_births','RESULT_deaths','END_bots','END_genomes')) {
                if ($pair.before[$key] -ne $pair.after[$key]) { throw "World state differs in $($case.Name): $key" }
            }
        }
    }
} finally { Pop-Location }
