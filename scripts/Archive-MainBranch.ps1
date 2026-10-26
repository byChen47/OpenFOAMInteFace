[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$repositoryRoot = (& git rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($repositoryRoot))
{
    throw 'The current directory is not inside a Git repository.'
}

$currentBranch = (& git branch --show-current).Trim()
if ($LASTEXITCODE -ne 0)
{
    throw 'Unable to determine the current Git branch.'
}

if ($currentBranch -ne 'main')
{
    throw "Archive workflow requires the current branch to be 'main'. Current branch: '$currentBranch'."
}

$headSha = (& git rev-parse --verify HEAD 2>$null)
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($headSha))
{
    throw 'The repository does not have an initial commit yet.'
}

$shortSha = (& git rev-parse --short HEAD).Trim()
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$archiveBranch = "archive/$timestamp-$shortSha"

& git show-ref --verify --quiet "refs/heads/$archiveBranch"
if ($LASTEXITCODE -eq 0)
{
    throw "Archive branch already exists: '$archiveBranch'."
}

& git branch $archiveBranch HEAD
if ($LASTEXITCODE -ne 0)
{
    throw "Failed to create archive branch '$archiveBranch'."
}

Write-Host "Archived previous main at branch '$archiveBranch'."
Write-Output $archiveBranch