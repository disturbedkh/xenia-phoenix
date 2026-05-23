# Validate metacache markdown links and stale ai_brain references.
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
)

$ErrorActionPreference = 'Stop'
$issues = @()
$rootNorm = $Root.TrimEnd('\') + '\'

Get-ChildItem -LiteralPath $Root -Recurse -File |
    Where-Object { $_.Extension -in '.md', '.yaml' -and $_.FullName -notmatch 'validate_links\.ps1' } |
    ForEach-Object {
        $rel = $_.FullName.Replace($rootNorm, '')
        $lines = Get-Content -LiteralPath $_.FullName

        if ($lines -match 'ai_brain') {
            $issues += "STALE ai_brain: $rel"
        }

        for ($i = 0; $i -lt $lines.Count; $i++) {
            $line = $lines[$i]
            foreach ($m in [regex]::Matches($line, '\]\(([^)]+)\)')) {
                $target = $m.Groups[1].Value
                if ($target -match '^(https?://|mailto:|file://|#)') { continue }
                $pathPart = ($target -split '#')[0]
                if ([string]::IsNullOrWhiteSpace($pathPart)) { continue }
                $resolved = if ([System.IO.Path]::IsPathRooted($pathPart)) {
                    $pathPart
                } else {
                    Join-Path $_.DirectoryName $pathPart
                }
                if (-not (Test-Path -LiteralPath $resolved)) {
                    $issues += "BROKEN ($rel`:$($i+1)): $target"
                }
            }
        }
    }

# Registry entry paths
$regPath = Join-Path $Root 'agents\registry.yaml'
if (Test-Path -LiteralPath $regPath) {
    $reg = Get-Content -LiteralPath $regPath -Raw
    foreach ($m in [regex]::Matches($reg, 'entry:\s*\n((?:\s+-\s+.+\n)+)')) {
        foreach ($e in [regex]::Matches($m.Groups[1].Value, '-\s+(.+)')) {
            $p = $e.Groups[1].Value.Trim()
            if ($p -match '^xenia-phoenix-src/') { continue }
            $full = Join-Path $Root $p
            if (-not (Test-Path -LiteralPath $full)) {
                $issues += "REGISTRY MISSING: $p"
            }
        }
    }
}

if ($issues.Count -eq 0) {
    Write-Host "validate_links: OK ($Root)"
    exit 0
}

$issues | ForEach-Object { Write-Warning $_ }
Write-Host "validate_links: $($issues.Count) issue(s)"
exit 1
