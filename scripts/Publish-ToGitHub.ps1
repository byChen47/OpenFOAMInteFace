[CmdletBinding()]
param(
    [string]$Message,

    [string]$Remote = 'origin'
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = (& git rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($repositoryRoot))
{
    throw 'The current directory is not inside a Git repository.'
}

$changes = @(& git status --porcelain)
if ($LASTEXITCODE -ne 0)
{
    throw 'Unable to read Git status.'
}

if ($changes.Count -eq 0)
{
    Write-Host 'No local changes to publish.'
    exit 0
}

if ([string]::IsNullOrWhiteSpace($Message))
{
    $timestamp = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
    $Message = "chore: publish update $timestamp"
}

$commitScript = Join-Path $PSScriptRoot 'Commit-Update.ps1'
$archiveBranch = (& $commitScript -Message $Message -Push -Remote $Remote).Trim()
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($archiveBranch))
{
    throw 'Publishing failed while archiving and committing the update.'
}

$localMain = (& git rev-parse HEAD).Trim()
$remoteHeads = @(& git ls-remote --heads $Remote)
if ($LASTEXITCODE -ne 0)
{
    throw "Unable to verify remote '$Remote' after publishing."
}

$mainLine = $remoteHeads | Where-Object { $_ -match "refs/heads/main$" } | Select-Object -First 1
$archiveLine = $remoteHeads | Where-Object { $_ -match [regex]::Escape("refs/heads/$archiveBranch") } | Select-Object -First 1

if ($null -eq $mainLine -or -not $mainLine.StartsWith($localMain))
{
    throw 'Remote main does not match the local main commit after publishing.'
}

if ($null -eq $archiveLine)
{
    throw "Remote archive branch '$archiveBranch' was not found after publishing."
}

Write-Host "Published main: $localMain"
Write-Host "Archived previous main: $archiveBranch"
Write-Output $archiveBranch