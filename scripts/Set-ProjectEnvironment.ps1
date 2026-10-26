# Dot-source this file, then call Set-ProjectEnvironment before CMake or application commands.

$ProjectRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$QtRoot = 'D:\3.wpsAndOther\Qt\setting\6.11.2\mingw_64'
$QtBin = Join-Path $QtRoot 'bin'
$VtkRoot = 'D:\3.wpsAndOther\VTK\install'
$VtkBin = Join-Path $VtkRoot 'bin'
$CMakeExe = 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\cmake.exe'
$CTestExe = 'D:\3.wpsAndOther\Qt\setting\Tools\CMake_64\bin\ctest.exe'
$NinjaExe = 'D:\3.wpsAndOther\Qt\setting\Tools\Ninja\ninja.exe'
$NinjaDir = Split-Path -Parent $NinjaExe
$MinGwBin = 'D:\3.wpsAndOther\Qt\setting\Tools\mingw1310_64\bin'
$ObjdumpExe = Join-Path $MinGwBin 'objdump.exe'
$WinDeployQtExe = Join-Path $QtBin 'windeployqt.exe'

function Get-ProjectToolchain
{
    return [pscustomobject]@{
        ProjectRoot = $ProjectRoot
        QtRoot = $QtRoot
        QtBin = $QtBin
        VtkRoot = $VtkRoot
        VtkBin = $VtkBin
        CMakeExe = $CMakeExe
        CTestExe = $CTestExe
        NinjaExe = $NinjaExe
        NinjaDir = $NinjaDir
        MinGwBin = $MinGwBin
        ObjdumpExe = $ObjdumpExe
        WinDeployQtExe = $WinDeployQtExe
    }
}

function Set-ProjectEnvironment
{
    $requiredPaths = @($QtBin, $MinGwBin, $NinjaDir, $VtkBin)
    $env:Path = ($requiredPaths -join ';') + ';' + $env:Path
}