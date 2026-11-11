param([string]$ExecutableDirectory = 'x64/ecology')
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$base = Join-Path $repoRoot $ExecutableDirectory
$outputDir = Join-Path $base 'validation'
New-Item -ItemType Directory -Force $outputDir | Out-Null
# Historical seed-12345, tick-450 worlds; hidden metadata must not alter them.
$expected = @{
    before = 'FDA6048AFFCF091850D75580AB9C31A98B40E1C0C1483311E93E80BF1DC53EAA'
    after = 'CBE6F68293A996DA81526AB34B9C062171BE7AF86F7B3C227747FC034181986E'
}
$records = @()
Push-Location $repoRoot
try {
    foreach ($variant in @('before','after')) {
        $world = Join-Path $outputDir "$variant.world"
        & (Join-Path $base "$variant/ecology.exe") --scenario normal --seed 12345 --ticks 450 --final-out $world `
            *> (Join-Path $outputDir "$variant.log")
        if ($LASTEXITCODE) { throw "Ecology validation process failed: $variant" }
        $actual = (Get-FileHash -LiteralPath $world -Algorithm SHA256).Hash
        if ($actual -ne $expected[$variant]) { throw "Ecology measurements changed the known $variant world." }
        $records += [pscustomobject]@{Variant=$variant; Tick=450; Seed=12345; SHA256=$actual}
        Write-Output "PASS ${variant}: identical historical world SHA-256."
    }
    $records | ConvertTo-Json | Set-Content (Join-Path $outputDir 'validation.json') -Encoding utf8
} finally { Pop-Location }
