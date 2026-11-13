param([string]$BeforeRevision = 'e3456b9', [string]$AfterRevision = '163e297',
      [string]$OutputDirectory = 'x64/ecology')
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$base = Join-Path $repoRoot $OutputDirectory
New-Item -ItemType Directory -Force $base | Out-Null
. (Join-Path $PSScriptRoot 'Find-VisualStudioV143.ps1')
$vs = Find-VisualStudioV143
if (!$vs) { throw 'v143 is required.' }
$vsRoot = $vs.Root
$version = $vs.Version
function Replace-Once($text, $old, $new) {
    if (($text.Split(@($old), [StringSplitOptions]::None).Count - 1) -ne 1) { throw "Missing/ambiguous anchor: $old" }
    return $text.Replace($old, $new)
}
Push-Location $repoRoot
try {
    foreach ($variant in @('before','after')) {
        $dir = Join-Path $base $variant
        New-Item -ItemType Directory -Force $dir | Out-Null
        $revision = if ($variant -eq 'before') { $BeforeRevision } else { $AfterRevision }
        $archive = Join-Path $dir 'source.tar'
        & git archive --format=tar "--output=$archive" $revision source
        if ($LASTEXITCODE) { throw "Cannot archive sources: $revision" }
        & (Join-Path $env:SystemRoot 'System32/tar.exe') -xf $archive -C $dir
        if ($LASTEXITCODE) { throw "Cannot extract sources: $revision" }
        $models = Join-Path $dir 'source/cb3'
        $fieldHeader = Join-Path $models 'Field.h'
        $text = (Get-Content $fieldHeader -Raw).Replace("`r`n", "`n")
        $text = Replace-Once $text "class Field final`n{" "class Field final`n{`n    friend struct EcologyProbe;"
        Set-Content $fieldHeader $text -Encoding utf8
        $fieldCpp = Join-Path $models 'Field.cpp'
        $text = (Get-Content $fieldCpp -Raw).Replace("`r`n", "`n")
        $text = '#include "CB3EcologyHooks.h"' + "`n" + $text
        $text = Replace-Once $text 'numThreads = std::clamp(cpuCount, 1, NumThreads);' 'numThreads = std::clamp(ecology::threads, 1, NumThreads);'
        Set-Content $fieldCpp $text -Encoding utf8
        $botHeader = Join-Path $models 'Bot.h'
        $text = (Get-Content $botHeader -Raw).Replace("`r`n", "`n")
        $text = Replace-Once $text "class Bot : public Object`n{" "class Bot : public Object`n{`npublic:`n    unsigned ecologyFounder = 0, ecologyTeam = 0;`nprivate:"
        Set-Content $botHeader $text -Encoding utf8
        $botCpp = Join-Path $models 'Bot.cpp'
        $text = (Get-Content $botCpp -Raw).Replace("`r`n", "`n")
        $text = '#include "CB3EcologyHooks.h"' + "`n" + $text
        $anchor = "Bot::Bot(int X, int Y, uint Energy, Bot* prototype, bool mutate) :Object(X, Y), genome(prototype->genome)`n{"
        $text = Replace-Once $text $anchor ($anchor + "`n    ecologyFounder = prototype->ecologyFounder; ecologyTeam = prototype->ecologyTeam;")
        $anchor = "Bot::Bot(int X, int Y, uint Energy) :Object(X, Y)`n{"
        $text = Replace-Once $text $anchor ($anchor + "`n    ecologyFounder = ecology::nextFounder++;")
        Set-Content $botCpp $text -Encoding utf8
        $compilerArguments = @('/nologo','/MP4','/MD','/EHsc','/std:c++20','/utf-8','/O2','/GL','/Zi','/DNDEBUG','/D_CRT_SECURE_NO_WARNINGS','/DSDL_MAIN_HANDLED',
            '/Itests','/Iexternal/SDL2/include','/Iexternal/SDL2_image/include','/Iimgui','/Iimgui/backends','/IIMplot',
            ('/I"{0}/source"' -f $dir), ('/Fo"{0}\\"' -f $dir), ('/Fd"{0}/compile.pdb"' -f $dir))
        if ($variant -eq 'after') { $compilerArguments += '/DECOLOGY_ADAPTIVE' }
        $compilerArguments += 'tests/CB3Ecology.cpp'
        $compilerArguments += @('Field','Bot','BotNeuralNet','Object','ObjectSaver','Neuron','Utils','Apple','Rock','Organics') | ForEach-Object { '"{0}/{1}.cpp"' -f $models,$_ }
        $compilerArguments += @('/link','/SUBSYSTEM:CONSOLE','/LTCG','/OPT:REF','/OPT:ICF','/DEBUG',('/OUT:"{0}/ecology.exe"' -f $dir),
            '/LIBPATH:external/SDL2/lib/x64','/LIBPATH:external/SDL2_image/lib/x64','SDL2.lib','SDL2_image.lib','user32.lib')
        $rsp = Join-Path $dir 'compile.rsp'
        ($compilerArguments -join ' ') | Set-Content $rsp -Encoding utf8
        $batch = Join-Path $dir 'compile.cmd'
        @('@echo off',('call "{0}" -no_logo -arch=x64 -vcvars_ver={1}' -f (Join-Path $vsRoot 'Common7/Tools/VsDevCmd.bat'),$version),
            'if errorlevel 1 exit /b 1',('cl.exe @"{0}"' -f $rsp),'exit /b %errorlevel%') | Set-Content $batch -Encoding ascii
        & $env:ComSpec /d /c "`"$batch`""
        if ($LASTEXITCODE) { throw "Ecology build failed: $variant" }
        Copy-Item -LiteralPath (Join-Path $repoRoot 'external/SDL2/lib/x64/SDL2.dll') -Destination $dir
    }
} finally { Pop-Location }
