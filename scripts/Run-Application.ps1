[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [switch]$SmokeTest,

    [string]$Screenshot
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Set-ProjectEnvironment.ps1')
$toolchain = Get-ProjectToolchain
Set-ProjectEnvironment | Out-Null

$configurationName = $Configuration.ToLowerInvariant()
$executable = Join-Path $toolchain.ProjectRoot "build\windows-mingw-$configurationName\OpenFOAMGUI.exe"

if (-not (Test-Path -LiteralPath $executable -PathType Leaf))
{
    throw "Application was not found at '$executable'. Run scripts\Build-Project.ps1 first."
}

$applicationArguments = @()
if ($SmokeTest -or -not [string]::IsNullOrWhiteSpace($Screenshot))
{
    $applicationArguments += '--smoke-test'
}

if (-not [string]::IsNullOrWhiteSpace($Screenshot))
{
    $applicationArguments += @('--screenshot', [System.IO.Path]::GetFullPath($Screenshot))
}

& $executable @applicationArguments
exit $LASTEXITCODE