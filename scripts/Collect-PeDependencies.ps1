function Get-PeDllNames
{
    param(
        [Parameter(Mandatory)]
        [string]$Path,

        [Parameter(Mandatory)]
        [string]$ObjdumpPath
    )

    $output = & $ObjdumpPath -p $Path
    if ($LASTEXITCODE -ne 0)
    {
        throw "objdump failed for '$Path' with exit code $LASTEXITCODE."
    }

    foreach ($line in $output)
    {
        if ($line -match '^\s*DLL Name:\s*(.+?)\s*$')
        {
            $Matches[1]
        }
    }
}

function Find-RuntimeDll
{
    param(
        [Parameter(Mandatory)]
        [string]$DllName,

        [Parameter(Mandatory)]
        [string[]]$SearchDirectories
    )

    foreach ($directory in $SearchDirectories)
    {
        $candidate = Join-Path $directory $DllName
        if (Test-Path -LiteralPath $candidate -PathType Leaf)
        {
            return $candidate
        }
    }

    return $null
}

function Copy-PeDependencyClosure
{
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]
        [string]$Executable,

        [Parameter(Mandatory)]
        [string]$Destination,

        [Parameter(Mandatory)]
        [string]$ObjdumpPath,

        [Parameter(Mandatory)]
        [string[]]$SearchDirectories
    )

    $destinationPath = [System.IO.Path]::GetFullPath($Destination)
    New-Item -ItemType Directory -Path $destinationPath -Force | Out-Null

    $pending = [System.Collections.Generic.Queue[string]]::new()
    $scheduled = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    $binariesSeen = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    $importsSeen = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)

    $executablePath = [System.IO.Path]::GetFullPath($Executable)
    $scheduled.Add($executablePath) | Out-Null
    $pending.Enqueue($executablePath)

    $orderedSearchDirectories = @($destinationPath) + $SearchDirectories
    $copied = [System.Collections.Generic.List[string]]::new()

    while ($pending.Count -gt 0)
    {
        $binary = $pending.Dequeue()
        if (-not $binariesSeen.Add($binary))
        {
            continue
        }

        foreach ($dllName in Get-PeDllNames -Path $binary -ObjdumpPath $ObjdumpPath)
        {
            if (-not $importsSeen.Add($dllName))
            {
                continue
            }

            $source = Find-RuntimeDll -DllName $dllName -SearchDirectories $orderedSearchDirectories
            if ($null -eq $source)
            {
                continue
            }

            $destinationFile = Join-Path $destinationPath (Split-Path -Leaf $source)
            if (-not (Test-Path -LiteralPath $destinationFile -PathType Leaf))
            {
                Copy-Item -LiteralPath $source -Destination $destinationFile
                $copied.Add((Split-Path -Leaf $source))
            }

            if ($scheduled.Add($destinationFile))
            {
                $pending.Enqueue($destinationFile)
            }
        }
    }

    return $copied
}