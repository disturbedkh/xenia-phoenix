# Smoke: develop preset creates events JSONL (requires built xenia + short run).
param(
    [string]$XeniaExe = "",
    [string]$TelemetryDir = "telemetry",
    [int]$TimeoutSec = 8
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo
. (Join-Path $PSScriptRoot "xenia_paths.ps1")
if (-not $XeniaExe) {
    $XeniaExe = Join-Path (Get-XeniaBinDir -Config Release) "xenia_canary.exe"
}

$eventsLog = Join-Path $TelemetryDir "00000000_events.jsonl"
Remove-Item -Force -ErrorAction SilentlyContinue $eventsLog
New-Item -ItemType Directory -Force -Path $TelemetryDir | Out-Null

if (-not (Test-Path $XeniaExe)) {
    Write-Host "SKIP: missing $XeniaExe (build xenia-app first)" -ForegroundColor Yellow
    exit 0
}

$proc = Start-Process -FilePath $XeniaExe -ArgumentList @(
    "--log_preset=develop",
    "--obs_events_log=$eventsLog"
) -PassThru -WorkingDirectory $repo

Start-Sleep -Seconds $TimeoutSec
if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
}

if (-not (Test-Path $eventsLog)) {
    Write-Host "FAIL: expected $eventsLog" -ForegroundColor Red
    exit 1
}
Write-Host "OK: $eventsLog created" -ForegroundColor Green
exit 0
