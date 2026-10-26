[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Set-ProjectEnvironment.ps1')
$toolchain = Get-ProjectToolchain
Set-ProjectEnvironment | Out-Null

$preset = "windows-mingw-$($Configuration.ToLowerInvariant())"

Write-Host "Configuring $Configuration build with preset '$preset'..."
& $toolchain.CMakeExe --preset $preset
if ($LASTEXITCODE -ne 0)
{
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

Write-Host "Building $Configuration..."
& $toolchain.CMakeExe --build --preset $preset
if ($LASTEXITCODE -ne 0)
{
    throw "CMake build failed with exit code $LASTEXITCODE."
}

if (-not $SkipTests)
{
    Write-Host "Running tests for $Configuration..."
    & $toolchain.CTestExe --preset $preset --output-on-failure
    if ($LASTEXITCODE -ne 0)
    {
        throw "CTest failed with exit code $LASTEXITCODE."
    }
}

Write-Host "Build completed successfully: $Configuration"