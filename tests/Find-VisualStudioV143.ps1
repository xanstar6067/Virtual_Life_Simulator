# Dot-source this file: . (Join-Path $PSScriptRoot 'Find-VisualStudioV143.ps1')
# Finds a Visual Studio installation with the v143 C++ toolset (MSVC 14.30-14.4x). The newest installation
# may not have it (e.g. VS 2026 next to Build Tools 2022), so every installation is checked.
function Find-VisualStudioV143 {
    $installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer'
    $vswhere = Join-Path $installer 'vswhere.exe'
    # VsDevCmd.bat of some installations calls vswhere.exe by name.
    if (($env:Path -split ';') -notcontains $installer) { $env:Path = "$installer;$env:Path" }
    foreach ($root in @(& $vswhere -all -products '*' -sort -property installationPath)) {
        $msvc = Join-Path $root 'VC\Tools\MSVC'
        if (!(Test-Path -LiteralPath $msvc)) { continue }
        $version = Get-ChildItem $msvc -Directory |
            Where-Object { [Version]$_.Name -ge [Version]'14.30' -and [Version]$_.Name -lt [Version]'14.50' } |
            Sort-Object { [Version]$_.Name } -Descending | Select-Object -First 1 -ExpandProperty Name
        if ($version) { return [pscustomobject]@{ Root = $root; Version = $version } }
    }
    return $null
}
