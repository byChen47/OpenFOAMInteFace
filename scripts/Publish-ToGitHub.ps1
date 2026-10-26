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

$previousMain = (& git rev-parse HEAD).Trim()
$archivesBefore = @(& git for-each-ref --format='%(refname:short)' --points-at $previousMain 'refs/heads/archive/*')
if ($LASTEXITCODE -ne 0)
{
    throw 'Unable to inspect archive branches before publishing.'
}

$commitScript = Join-Path $PSScriptRoot 'Commit-Update.ps1'
& $commitScript -Message $Message -Push -Remote $Remote
if ($LASTEXITCODE -ne 0)
{
    throw 'Publishing failed while archiving and committing the update.'
}

$localMain = (& git rev-parse HEAD).Trim()
$archivesAfter = @(& git for-each-ref --format='%(refname:short)' --points-at $previousMain 'refs/heads/archive/*')
if ($LASTEXITCODE -ne 0)
{
    throw 'Unable to inspect archive branches after publishing.'
}

$newArchives = @($archivesAfter | Where-Object { $archivesBefore -notcontains $_ })
if ($newArchives.Count -ne 1)
{
    throw "Expected one new archive branch, found $($newArchives.Count)."
}

$archiveBranch = $newArchives[0]
$remoteHeads = @(& git ls-remote --heads $Remote)
if ($LASTEXITCODE -ne 0)
{
    throw "Unable to inspect remote '$Remote' after publishing."
}

$mainLine = @($remoteHeads | Where-Object { $_.EndsWith('refs/heads/main') } | Select-Object -First 1)
if ($mainLine.Count -eq 0)
{
    throw 'Remote main was not found after publishing.'
}

$remoteMain = ($mainLine[0] -split '\s+')[0]
if ($remoteMain -ne $localMain)
{
    throw 'Remote main does not match the local main commit after publishing.'
}

$archiveRef = "refs/heads/$archiveBranch"
$archiveLine = @($remoteHeads | Where-Object { $_.EndsWith($archiveRef) } | Select-Object -First 1)
if ($archiveLine.Count -eq 0)
{
    throw "Remote archive branch '$archiveBranch' was not found after publishing."
}

Write-Host "Published main: $localMain"
Write-Host "Archived previous main: $archiveBranch"
Write-Output $archiveBranch