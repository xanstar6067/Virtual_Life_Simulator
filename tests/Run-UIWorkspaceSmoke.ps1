param([switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -property installationPath
$msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\MSBuild.exe'
$devCmd = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
$compilerVersion = Get-ChildItem (Join-Path $vsRoot 'VC\Tools\MSVC') -Directory |
    Where-Object { [Version]$_.Name -ge [Version]'14.30' -and [Version]$_.Name -lt [Version]'14.50' } |
    Sort-Object { [Version]$_.Name } -Descending | Select-Object -First 1 -ExpandProperty Name
if (!$compilerVersion) { throw 'The project requires the v143 C++ toolset.' }

Push-Location $repoRoot
try {
    if (!$SkipBuild) {
        & $msbuild VirtualLifeSimulator1.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
        if ($LASTEXITCODE) { throw 'Debug build failed.' }
    }
    $outputDir = Join-Path $repoRoot 'x64\Debug'
    $objectDir = Join-Path $repoRoot 'VirtualL.b8f19b53\x64\Debug'
    [xml]$project = Get-Content -LiteralPath (Join-Path $repoRoot 'VirtualLifeSimulator1.vcxproj') -Raw
    $objects = foreach ($source in $project.SelectNodes("//*[local-name()='ClCompile' and @Include]")) {
        $name = [IO.Path]::GetFileNameWithoutExtension($source.Include) + '.obj'
        if ($source.Include.StartsWith('source\cb3\')) { $name = Join-Path 'cb3' $name }
        Get-Item -LiteralPath (Join-Path $objectDir $name)
    }
    if (!$objects) { throw 'Build Debug x64 before running the tests.' }
    foreach ($mode in @('classic', 'cb3')) {
        $testExe = Join-Path $outputDir "ui-smoke-$mode.exe"
        $responsePath = Join-Path $outputDir "ui-smoke-$mode.rsp"
        $batchPath = Join-Path $outputDir "ui-smoke-$mode.cmd"
        $artifactDir = Join-Path $outputDir "ui-smoke-$mode"
        $arguments = @('/nologo', '/MDd', '/EHsc', '/std:c++20', '/utf-8', '/Zi', '/W3',
            '/D_CRT_SECURE_NO_WARNINGS', '/DSDL_MAIN_HANDLED',
            ('/Fo"{0}"' -f (Join-Path $outputDir "ui-smoke-$mode.obj")),
            ('/Fd"{0}"' -f (Join-Path $outputDir "ui-smoke-$mode-compile.pdb")),
            '/Iexternal\SDL2\include', '/Iexternal\SDL2_image\include', '/Iimgui', '/Iimgui\backends', '/IIMplot')
        if ($mode -eq 'cb3') { $arguments += '/DTEST_CB3' }
        $arguments += 'tests\UIWorkspaceSmoke.cpp'
        $arguments += $objects | ForEach-Object { '"{0}"' -f $_.FullName }
        $arguments += @('/link', '/SUBSYSTEM:CONSOLE', '/ENTRY:mainCRTStartup',
            ('/OUT:"{0}"' -f $testExe), ('/PDB:"{0}"' -f (Join-Path $outputDir "ui-smoke-$mode.pdb")),
            '/LIBPATH:external\SDL2\lib\x64', '/LIBPATH:external\SDL2_image\lib\x64',
            'SDL2.lib', 'SDL2_image.lib', 'opengl32.lib', 'shell32.lib', 'user32.lib', 'gdi32.lib', 'imm32.lib')
        ($arguments -join ' ') | Set-Content -LiteralPath $responsePath -Encoding utf8
        @('@echo off', ('call "{0}" -no_logo -arch=x64 -vcvars_ver={1}' -f $devCmd, $compilerVersion),
            'if errorlevel 1 exit /b 1', ('cl.exe @"{0}"' -f $responsePath), 'exit /b %errorlevel%') |
            Set-Content -LiteralPath $batchPath -Encoding ascii
        & $env:ComSpec /d /c "`"$batchPath`""
        if ($LASTEXITCODE) { throw "$mode smoke test compilation failed." }
        & $testExe $artifactDir
        if ($LASTEXITCODE) { throw "$mode smoke test failed." }
    }
} finally {
    Pop-Location
}
