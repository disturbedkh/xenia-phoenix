# Poll phoenix_probe + tail telemetry while a title runs.
param(
    [Parameter(Mandatory = $true)]
    [string]$TitleId,
    [int]$Port = 0,
    [int]$IntervalSec = 5,
    [int]$Count = 12,
    [string]$TelemetryDir = "telemetry"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

if ($Port -le 0) {
    if ($env:PHOENIX_DEBUG_PORT) { $Port = [int]$env:PHOENIX_DEBUG_PORT }
    else { $Port = 8765 }
}

$tid = $TitleId.ToUpper().Replace("0x", "")
Write-Host "=== Live probe ($tid) port $Port every ${IntervalSec}s x $Count ===" -ForegroundColor Cyan

for ($i = 1; $i -le $Count; $i++) {
    Write-Host ""
    Write-Host "--- sample $i / $Count $(Get-Date -Format 'HH:mm:ss') ---" -ForegroundColor Yellow
    phoenixctl --json probe snapshot --port $Port 2>&1
    phoenixctl tail pcm --title-id $TitleId --lines 3 2>&1
    phoenixctl tail stubs --title-id $TitleId --lines 3 2>&1
    if ($i -lt $Count) { Start-Sleep -Seconds $IntervalSec }
}

Write-Host ""
Write-Host "Done. Post-session: phoenixctl triage --title-id $TitleId"
