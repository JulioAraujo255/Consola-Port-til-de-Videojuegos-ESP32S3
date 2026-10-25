$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Se requiere Visual Studio Build Tools con C++.' }
$vcvars = Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'
Push-Location $projectRoot
try {
    New-Item -ItemType Directory -Force -Path '.pio/host-tests' | Out-Null
    foreach ($suite in 'snake', 'dino', 'carreras', 'consola', 'brasa', 'cuadro', 'controles') {
        $command = 'call "{0}" >nul && cl /nologo /std:c++17 /EHsc /W3 /D_CRT_SECURE_NO_WARNINGS /Itest\host /Iinclude test\host\{1}_test.cpp /Fe:.pio\host-tests\{1}_test.exe /Fo:.pio\host-tests\{1}_test.obj && .pio\host-tests\{1}_test.exe' -f $vcvars, $suite
        & cmd.exe /d /c $command
        if ($LASTEXITCODE -ne 0) { throw "Falló la suite $suite" }
    }
} finally { Pop-Location }
