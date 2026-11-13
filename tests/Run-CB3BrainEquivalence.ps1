param([switch]$DebugBuild)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$out = Join-Path $repoRoot 'x64\brain-equivalence'
New-Item -ItemType Directory -Path $out -Force | Out-Null
. (Join-Path $PSScriptRoot 'Find-VisualStudioV143.ps1')
$vs = Find-VisualStudioV143
if (!$vs) { throw 'The tests require v143.' }
$vsRoot = $vs.Root
$version = $vs.Version
$compileFlags = if ($DebugBuild) { '/MDd /Od /RTC1 /D_DEBUG' } else { '/MD /O2 /GL /DNDEBUG' }
$linkFlags = if ($DebugBuild) { '' } else { '/LTCG' }
$compilerArguments = @('/nologo', '/MP4', '/EHsc', '/std:c++20', '/utf-8', '/Zi', $compileFlags,
    '/D_CRT_SECURE_NO_WARNINGS', '/DSDL_MAIN_HANDLED', '/Iexternal\SDL2\include',
    '/Iimgui', '/Iimgui\backends', '/IIMplot', ('/Fo"{0}\\"' -f $out), ('/Fd"{0}\compile.pdb"' -f $out),
    'tests\CB3BrainEquivalence.cpp', 'source\cb3\BotNeuralNet.cpp', 'source\cb3\Neuron.cpp', 'source\cb3\Utils.cpp',
    '/link', $linkFlags, '/DEBUG', '/SUBSYSTEM:CONSOLE', ('/OUT:"{0}\equivalence.exe"' -f $out))
$rsp = Join-Path $out 'compile.rsp'
($compilerArguments -join ' ') | Set-Content -LiteralPath $rsp -Encoding utf8
$batch = Join-Path $out 'compile.cmd'
@('@echo off', ('call "{0}" -no_logo -arch=x64 -vcvars_ver={1}' -f (Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'), $version),
    'if errorlevel 1 exit /b 1', ('cl.exe @"{0}"' -f $rsp), 'exit /b %errorlevel%') | Set-Content -LiteralPath $batch -Encoding ascii
Push-Location $repoRoot
try {
    & $env:ComSpec /d /c "`"$batch`""
    if ($LASTEXITCODE) { throw 'Equivalence test compilation failed.' }
    & (Join-Path $out 'equivalence.exe')
    if ($LASTEXITCODE) { throw 'Brain execution differs from the reference.' }
} finally { Pop-Location }
