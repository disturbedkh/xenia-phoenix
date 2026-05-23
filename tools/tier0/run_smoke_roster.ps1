# Run 5-minute smoke capture for every title in telemetry/smoke_roster_local.toml
# Copy metacache/dev/smoke_roster_local.example.toml to that path and edit game_path values.
param(
    [string]$ConfigPath = "telemetry/smoke_roster_local.toml",
    [switch]$AggregateOnly
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

if (-not (Test-Path $ConfigPath)) {
    Write-Error @"
Missing $ConfigPath
Copy Xenia-Phoenix/metacache/dev/smoke_roster_local.example.toml to telemetry/smoke_roster_local.toml and set game_path per title.
"@
}

function Get-TomlValue([string]$block, [string]$key) {
    if ($block -match "(?m)^\s*$key\s*=\s*""([^""]*)""") { return $Matches[1] }
    if ($block -match "(?m)^\s*$key\s*=\s*(\S+)") { return $Matches[1].Trim('"') }
    return $null
}

$text = Get-Content -Raw $ConfigPath
$exe = "build/bin/Windows/Release/xenia_canary.exe"
$duration = 300
$hid = ""
if ($text -match '(?s)\[xenia\](.*?)(?=\[\[title\]|$)') {
    $x = $Matches[1]
    $v = Get-TomlValue $x "exe"; if ($v) { $exe = $v }
    $v = Get-TomlValue $x "duration_sec"; if ($v) { $duration = [int]$v }
    $v = Get-TomlValue $x "hid"; if ($v) { $hid = $v }
}

$blocks = [regex]::Matches($text, '(?s)\[\[title\]\](.*?)(?=\[\[title\]|$)')
$titles = @()
foreach ($m in $blocks) {
    $b = $m.Groups[1].Value
    $id = Get-TomlValue $b "id"
    $path = Get-TomlValue $b "game_path"
    if ($id -and $path) {
        $titles += [pscustomobject]@{ Id = $id; Path = $path }
    }
}

if ($titles.Count -eq 0) {
    Write-Error "No [[title]] blocks with id + game_path in $ConfigPath"
}

if (-not $AggregateOnly) {
    foreach ($t in $titles) {
        if (-not (Test-Path $t.Path)) {
            Write-Warning "Skip $($t.Id): game_path not found: $($t.Path)"
            continue
        }
        $args = @{
            TitleId    = $t.Id
            GamePath   = $t.Path
            XeniaExe   = $exe
            DurationSec = $duration
        }
        if ($hid) { $args.Hid = $hid }
        & "$PSScriptRoot\run_smoke_capture.ps1" @args
    }
}

$stubLogs = @()
foreach ($t in $titles) {
    $tid = $t.Id.ToLower().Replace("0x", "")
    $p = Join-Path "telemetry" "${tid}_stubs.jsonl"
    if (Test-Path $p) { $stubLogs += $p }
}

if ($stubLogs.Count -gt 0) {
    python tools/tier0/aggregate_stub_hits.py @stubLogs --out telemetry/smoke_stub_summary.json
    Write-Host "Wrote telemetry/smoke_stub_summary.json ($($stubLogs.Count) titles)"
} else {
    Write-Warning "No stub JSONL files to aggregate."
}
