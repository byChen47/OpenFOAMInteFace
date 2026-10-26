[CmdletBinding()]
param(
    [string]$Destination,

    [switch]$BuildFirst,

    [switch]$SkipSmokeTest
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Set-ProjectEnvironment.ps1')
. (Join-Path $PSScriptRoot 'Collect-PeDependencies.ps1')

$toolchain = Get-ProjectToolchain
Set-ProjectEnvironment | Out-Null

if ($BuildFirst)
{
    & (Join-Path $PSScriptRoot 'Build-Project.ps1') -Configuration Release
    if ($LASTEXITCODE -ne 0)
    {
        throw "Release build failed with exit code $LASTEXITCODE."
    }
}

if ([string]::IsNullOrWhiteSpace($Destination))
{
    $Destination = Join-Path $toolchain.ProjectRoot 'dist\OpenFOAMGUI'
}

$destinationPath = [System.IO.Path]::GetFullPath($Destination)
$projectRootWithSeparator = $toolchain.ProjectRoot.TrimEnd('\') + '\'
if (-not $destinationPath.StartsWith($projectRootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase))
{
    throw "Deployment destination must be inside the project directory: $destinationPath"
}

$releaseExecutable = Join-Path $toolchain.ProjectRoot 'build\windows-mingw-release\OpenFOAMGUI.exe'
if (-not (Test-Path -LiteralPath $releaseExecutable -PathType Leaf))
{
    throw "Release executable was not found at '$releaseExecutable'. Use -BuildFirst or build Release first."
}

New-Item -ItemType Directory -Path $destinationPath -Force | Out-Null
$deployedExecutable = Join-Path $destinationPath 'OpenFOAMGUI.exe'
Copy-Item -LiteralPath $releaseExecutable -Destination $deployedExecutable -Force

Write-Host "Deploying Qt runtime with windeployqt..."
& $toolchain.WinDeployQtExe `
    --release `
    --compiler-runtime `
    --no-translations `
    --dir $destinationPath `
    $deployedExecutable
if ($LASTEXITCODE -ne 0)
{
    throw "windeployqt failed with exit code $LASTEXITCODE."
}

Write-Host "Collecting transitive VTK and MinGW DLL dependencies..."
$copiedDependencies = Copy-PeDependencyClosure `
    -Executable $deployedExecutable `
    -Destination $destinationPath `
    -ObjdumpPath $toolchain.ObjdumpExe `
    -SearchDirectories @($toolchain.QtBin, $toolchain.MinGWBin, $toolchain.VtkBin)

Write-Host "Additional DLLs copied: $($copiedDependencies.Count)"

if (-not $SkipSmokeTest)
{
    $screenshotPath = Join-Path $destinationPath 'deployment-smoke.png'
    $originalPath = $env:Path

    try
    {
        # Only Windows system directories are visible. Application DLLs must be found
        # next to the executable or in its deployed plugin directories.
        $env:Path = "$env:SystemRoot\System32;$env:SystemRoot"

        Write-Host "Running deployed application with a minimal PATH..."
        & $deployedExecutable --smoke-test --screenshot $screenshotPath
        if ($LASTEXITCODE -ne 0)
        {
            throw "Deployed application smoke test failed with exit code $LASTEXITCODE."
        }

        if (-not (Test-Path -LiteralPath $screenshotPath -PathType Leaf))
        {
            throw "Deployment smoke test did not create a screenshot."
        }
    }
    finally
    {
        $env:Path = $originalPath
    }
}

$fileCount = (Get-ChildItem -LiteralPath $destinationPath -Recurse -File).Count
$totalBytes = (Get-ChildItem -LiteralPath $destinationPath -Recurse -File | Measure-Object Length -Sum).Sum
Write-Host "Deployment completed: $destinationPath"
Write-Host "Files: $fileCount, size: $([math]::Round($totalBytes / 1MB, 1)) MB"