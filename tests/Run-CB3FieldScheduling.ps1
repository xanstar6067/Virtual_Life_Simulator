$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'Build-CB3Performance.ps1') -SchedulingTests
Push-Location $repoRoot
try {
    & (Join-Path $repoRoot 'x64/perf-audit/scheduling-tests/cb3-perf.exe')
    if ($LASTEXITCODE) { throw 'CB3 field scheduling regression.' }
} finally { Pop-Location }
