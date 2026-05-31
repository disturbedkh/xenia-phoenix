# Smoke test phoenix_probe HTTP (requires xenia built with phoenix_debug_port support).
param(
    [int]$Port = 0,
    [int]$WaitSec = 12,
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

if ($Port -le 0) {
    if ($env:PHOENIX_DEBUG_PORT) { $Port = [int]$env:PHOENIX_DEBUG_PORT }
    else { $Port = 8765 }
}

. (Join-Path $PSScriptRoot "xenia_paths.ps1")
$exe = Join-Path (Get-XeniaBinDir -Config $Config) "xenia_canary.exe"
if (-not (Test-Path $exe)) {
    Write-Error "Missing $exe - build xenia-app first."
}

Get-Process xenia_canary -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

$log = Join-Path $env:TEMP "phoenix_probe_smoke.log"
$p = Start-Process -FilePath (Resolve-Path $exe).Path -ArgumentList @(
    "--phoenix_debug_port=$Port",
    "--log_file=$log"
) -PassThru

Start-Sleep -Seconds $WaitSec

$failed = $false
try {
    $health = Invoke-WebRequest -Uri "http://127.0.0.1:$Port/health" -UseBasicParsing -TimeoutSec 3
    if ($health.StatusCode -ne 200 -or $health.Content -notmatch '"ok"\s*:\s*true') {
        Write-Host "FAIL /health: $($health.StatusCode) $($health.Content)" -ForegroundColor Red
        $failed = $true
    } else {
        Write-Host "OK /health: $($health.Content)"
    }

    $status = Invoke-WebRequest -Uri "http://127.0.0.1:$Port/status" -UseBasicParsing -TimeoutSec 3
    if ($status.StatusCode -ne 200 -or $status.Content -notmatch 'uptime_ms') {
        Write-Host "FAIL /status: $($status.StatusCode) $($status.Content)" -ForegroundColor Red
        $failed = $true
    } else {
        Write-Host "OK /status: $($status.Content)"
    }

    $snap = Invoke-WebRequest -Uri "http://127.0.0.1:$Port/snapshot" -UseBasicParsing -TimeoutSec 3
    if ($snap.StatusCode -ne 200 -or $snap.Content -notmatch '"ownership_change_count"') {
        Write-Host "FAIL /snapshot: $($snap.StatusCode) $($snap.Content)" -ForegroundColor Red
        $failed = $true
    } else {
        $len = [Math]::Min(120, $snap.Content.Length)
        Write-Host "OK /snapshot (truncated): $($snap.Content.Substring(0, $len))..."
    }
} catch {
    Write-Host "FAIL probe request: $_" -ForegroundColor Red
    $failed = $true
}

if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }

if ($failed) { exit 1 }
Write-Host "probe_http_smoke: PASS" -ForegroundColor Green
exit 0
