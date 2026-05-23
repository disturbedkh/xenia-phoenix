# Unified smoke capture: kernel stubs + APU XMA divergences + PCM hash (Phase 1.4).
param(
    [Parameter(Mandatory = $true)]
    [string]$TitleId,
    [Parameter(Mandatory = $true)]
    [string]$GamePath,
    [string]$XeniaExe = "build\bin\Windows\Release\xenia_canary.exe",
    [int]$DurationSec = 300,
    [string]$TelemetryDir = "telemetry",
    [string]$LogFile = "",
    [string]$Hid = "",
    [string]$TraceGpuPrefix = ""
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

$tid = $TitleId.ToLower().Replace("0x", "")
$stubLog = Join-Path $TelemetryDir "${tid}_stubs.jsonl"
$eventsLog = Join-Path $TelemetryDir "${tid}_events.jsonl"
$xmaLog = Join-Path $TelemetryDir "${tid}_xma.jsonl"
$pcmLog = Join-Path $TelemetryDir "${tid}_pcm.jsonl"

New-Item -ItemType Directory -Force -Path $TelemetryDir | Out-Null
Remove-Item -Force -ErrorAction SilentlyContinue $stubLog, $xmaLog, $pcmLog

$gameQuoted = "`"$GamePath`""
$launchArgs = @(
    $gameQuoted,
    "--log_preset=develop",
    "--kernel_stub_hit_log=$stubLog",
    "--obs_events_log=$eventsLog",
    "--apu_xma_divergence_log=$xmaLog",
    "--apu_pcm_hash_log=$pcmLog"
)
if ($Hid) { $launchArgs += "--hid=$Hid" }
if ($LogFile) { $launchArgs += "--log_file=$LogFile" }
if ($TraceGpuPrefix) {
    New-Item -ItemType Directory -Force -Path $TraceGpuPrefix | Out-Null
    $launchArgs += "--trace_gpu_prefix=$TraceGpuPrefix"
}
if ($env:PHOENIX_DEBUG_PORT) {
    $launchArgs += "--phoenix_debug_port=$($env:PHOENIX_DEBUG_PORT)"
}

Write-Host "Launching smoke capture for $TitleId ($DurationSec s)..."
$p = Start-Process -FilePath $XeniaExe -ArgumentList $launchArgs -PassThru -WorkingDirectory $repo
Start-Sleep -Seconds $DurationSec
if (-not $p.HasExited) {
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
}

Write-Host "Aggregating telemetry..."
python tools/tier0/aggregate_stub_hits.py $stubLog --out (Join-Path $TelemetryDir "${tid}_stub_summary.json")
if (Test-Path $xmaLog) {
    python tools/tier0/aggregate_xma_divergences.py $xmaLog --out (Join-Path $TelemetryDir "${tid}_xma_summary.json")
}

$baseline = Join-Path $TelemetryDir "${tid}_pcm_baseline.jsonl"
if (-not (Test-Path $baseline)) {
    $baseline = Join-Path $TelemetryDir "${tid}_30s_pcm_baseline.jsonl"
}
if (Test-Path $baseline) {
    python tools/tier0/compare_pcm_hash_log.py $pcmLog $baseline
} else {
    Write-Host "No PCM baseline at ${tid}_pcm_baseline.jsonl - run compare_pcm_hash_log.py --update-baseline after first capture"
}

if (Test-Path (Join-Path $PSScriptRoot "triage_gameplay_capture.ps1")) {
    & "$PSScriptRoot\triage_gameplay_capture.ps1" -TitleId $TitleId -TelemetryDir $TelemetryDir
}

Write-Host "Done. Logs under $TelemetryDir"
