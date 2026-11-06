param([switch]$Profile)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$variant = if ($Profile) { 'profile' } else { 'baseline' }
$outputDir = Join-Path $repoRoot "x64\perf-audit\$variant"
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

function Replace-Once([string]$Text, [string]$Old, [string]$New) {
    if (($Text.Split(@($Old), [StringSplitOptions]::None).Count - 1) -ne 1) {
        throw "Instrumentation anchor is missing or ambiguous: $Old"
    }
    return $Text.Replace($Old, $New)
}
function Instrument-Function([string]$Text, [string]$Signature, [string]$Statement) {
    return Replace-Once $Text ($Signature + "`n{") ($Signature + "`n{`n" + $Statement)
}

# Generate copies under ignored build output. Never patch production sources.
$field = (Get-Content -LiteralPath (Join-Path $repoRoot 'source\cb3\Field.cpp') -Raw).Replace("`r`n", "`n")
$field = '#include "CB3PerformanceHooks.h"' + "`n" + $field
$field = Replace-Once $field 'numThreads = std::clamp(cpuCount, 1, NumThreads);' 'numThreads = std::clamp(audit::requestedThreads, 1, NumThreads);'
$field = Instrument-Function $field 'void Field::ProcessPart_MultipleThreads(const uint X1, const uint X2, const uint index)' '    audit::slot = int(index);'
$field = Replace-Once $field '            //Calculate chunk' "            const auto auditStart = audit::Now();`n            //Calculate chunk"
# The exact following region occurs once at the end of a worker pass.
$anchor = "            {`n                std::lock_guard<std::mutex> lock(threadMutex);`n                threadGoMarker[index] = false;"
$field = Replace-Once $field $anchor ("            audit::workers[index].phase[pass] += audit::Now() - auditStart;`n            audit::workers[index].objects[pass] += objectCounters[index].objects;`n" + $anchor)
Set-Content -LiteralPath (Join-Path $outputDir 'Field.cpp') -Value $field -Encoding utf8

$bot = (Get-Content -LiteralPath (Join-Path $repoRoot 'source\cb3\Bot.cpp') -Raw).Replace("`r`n", "`n")
$bot = '#include "CB3PerformanceHooks.h"' + "`n" + $bot
$bot = Replace-Once $bot 'tmpOut = think(input);' "tmpOut = audit::skipBrain ? BrainOutput{} : think(input);`n    if (audit::freeze) tmpOut = {};"
if ($Profile) {
    $bot = Instrument-Function $bot 'int Bot::tick()' '    audit::SampleBot auditSample;'
    $bot = Instrument-Function $bot 'BrainInput Bot::FillBrainInput()' '    audit::Scope auditSense(audit::Sense);'
    $bot = Instrument-Function $bot 'BrainOutput Bot::think(BrainInput input)' '    audit::Scope auditThink(audit::Think);'
    $bot = Instrument-Function $bot 'void Bot::Multiply(int numChildren, float energy_to_pass)' '    audit::Scope auditMultiply(audit::Multiply);'
    $bot = Instrument-Function $bot 'void Bot::Mutate()' '    audit::Scope auditMutate(audit::Mutate);'
}
Set-Content -LiteralPath (Join-Path $outputDir 'Bot.cpp') -Value $bot -Encoding utf8
$brain = (Get-Content -LiteralPath (Join-Path $repoRoot 'source\cb3\BotNeuralNet.cpp') -Raw).Replace("`r`n", "`n")
$brain = '#include "CB3PerformanceHooks.h"' + "`n" + $brain
if ($Profile) { $brain = Instrument-Function $brain 'void BrainGenome::RebuildActive()' '    audit::Scope auditRebuild(audit::Rebuild);' }
Set-Content -LiteralPath (Join-Path $outputDir 'BotNeuralNet.cpp') -Value $brain -Encoding utf8

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -property installationPath
$devCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
$compilerVersion = Get-ChildItem (Join-Path $vsRoot 'VC\Tools\MSVC') -Directory |
    Where-Object { [Version]$_.Name -ge [Version]'14.30' -and [Version]$_.Name -lt [Version]'14.50' } |
    Sort-Object { [Version]$_.Name } -Descending | Select-Object -First 1 -ExpandProperty Name
if (!$compilerVersion) { throw 'The benchmark requires the v143 C++ toolset.' }
$arguments = @('/nologo', '/MP4', '/MD', '/EHsc', '/std:c++20', '/utf-8', '/O2', '/GL', '/Zi', '/DNDEBUG', '/D_CRT_SECURE_NO_WARNINGS', '/DSDL_MAIN_HANDLED',
    '/Itests', '/Iexternal\SDL2\include', '/Iexternal\SDL2_image\include', '/Iimgui', '/Iimgui\backends', '/IIMplot', '/Isource\cb3',
    ('/Fo"{0}\\"' -f $outputDir), ('/Fd"{0}\compile.pdb"' -f $outputDir))
if ($Profile) { $arguments += '/DCB3_AUDIT_PROFILE' }
$arguments += @('tests\CB3Performance.cpp', ('"{0}\Field.cpp"' -f $outputDir), ('"{0}\Bot.cpp"' -f $outputDir), ('"{0}\BotNeuralNet.cpp"' -f $outputDir))
$arguments += @('Object', 'ObjectSaver', 'Neuron', 'Utils', 'Apple', 'Rock', 'Organics') | ForEach-Object { "source\cb3\$_.cpp" }
$arguments += @('/link', '/SUBSYSTEM:CONSOLE', '/LTCG', '/OPT:REF', '/OPT:ICF', '/DEBUG', ('/OUT:"{0}\cb3-perf.exe"' -f $outputDir),
    '/LIBPATH:external\SDL2\lib\x64', '/LIBPATH:external\SDL2_image\lib\x64', 'SDL2.lib', 'SDL2_image.lib', 'psapi.lib', 'user32.lib')
$responsePath = Join-Path $outputDir 'compile.rsp'
($arguments -join ' ') | Set-Content -LiteralPath $responsePath -Encoding utf8
$batchPath = Join-Path $outputDir 'compile.cmd'
@('@echo off', ('call "{0}" -no_logo -arch=x64 -vcvars_ver={1}' -f $devCmd, $compilerVersion), 'if errorlevel 1 exit /b 1',
    ('cl.exe @"{0}"' -f $responsePath), 'exit /b %errorlevel%') | Set-Content -LiteralPath $batchPath -Encoding ascii
Push-Location $repoRoot
try {
    & $env:ComSpec /d /c "`"$batchPath`""
    if ($LASTEXITCODE) { throw 'Benchmark compilation failed.' }
    Copy-Item -LiteralPath (Join-Path $repoRoot 'external\SDL2\lib\x64\SDL2.dll') -Destination $outputDir
} finally { Pop-Location }
Write-Output (Join-Path $outputDir 'cb3-perf.exe')
