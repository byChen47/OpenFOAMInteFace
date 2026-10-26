[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$Message,

    [switch]$Push,

    [string]$Remote = 'origin'
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = (& git rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($repositoryRoot))
{
    throw 'The current directory is not inside a Git repository.'
}

$currentBranch = (& git branch --show-current).Trim()
if ($currentBranch -ne 'main')
{
    throw "Updates must be committed on 'main'. Current branch: '$currentBranch'."
}

if ([string]::IsNullOrWhiteSpace($Message))
{
    throw 'A non-empty commit message is required.'
}

$changes = @(& git status --porcelain)
if ($LASTEXITCODE -ne 0)
{
    throw 'Unable to read Git status.'
}

if ($changes.Count -eq 0)
{
    throw 'No changes to commit.'
}

if ($Push)
{
    & git remote get-url $Remote *> $null
    if ($LASTEXITCODE -ne 0)
    {
        throw "Git remote '$Remote' is not configured. Add a remote or omit -Push."
    }
}

$archiveBranch = (& (Join-Path $PSScriptRoot 'Archive-MainBranch.ps1')).Trim()
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($archiveBranch))
{
    throw 'Failed to archive the previous main branch.'
}

& git add -A
if ($LASTEXITCODE -ne 0)
{
    throw 'git add failed.'
}

& git commit -m $Message
if ($LASTEXITCODE -ne 0)
{
    throw 'git commit failed.'
}

Write-Host "Committed update to main. Previous main is preserved at '$archiveBranch'."

if ($Push)
{
    & git push $Remote $archiveBranch main
    if ($LASTEXITCODE -ne 0)
    {
        throw "Commit succeeded locally, but pushing '$archiveBranch' and 'main' failed."
    }

    Write-Host "Pushed '$archiveBranch' and 'main' to remote '$Remote'."
}

Write-Output $archiveBranch