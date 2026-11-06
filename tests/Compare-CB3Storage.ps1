param([int]$Repeats = 3,
      [string]$BeforeExecutable = 'x64/perf-audit/before-storage/cb3-perf.exe',
      [string]$AfterExecutable = 'x64/perf-audit/baseline/cb3-perf.exe')
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'Compare-CB3NoopMutation.ps1') -Repeats $Repeats `
    -BeforeExecutable $BeforeExecutable -AfterExecutable $AfterExecutable `
    -OutputDirectory 'x64/perf-audit/storage-comparison' -ResultsCsv 'CB3_STORAGE_RESULTS.csv' `
    -RequireSameGenomes
