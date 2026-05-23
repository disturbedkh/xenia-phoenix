# Instrumented BF2 launch for post-stub crash triage (manual play to crash).
param(
    [Parameter(Mandatory = $true)]
    [string]$GamePath,
    [string]$TitleId = "454107DB",
    [ValidateSet("Auto", "Debug", "Release")]
    [string]$Configuration = "Auto",
    [string]$XeniaExe = "",
    [string]$TelemetryDir = "telemetry",
    [int]$DurationSec = 0,
    [switch]$FreshSession
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

$tid = $TitleId.ToLower().Replace("0x", "")
$stubLog = Join-Path $TelemetryDir "${tid}_stubs.jsonl"
$xmaLog = Join-Path $TelemetryDir "${tid}_xma.jsonl"
$pcmLog = Join-Path $TelemetryDir "${tid}_pcm.jsonl"
$crashLog = Join-Path $TelemetryDir "${tid}_crash.log"
$gpuTrace = Join-Path $TelemetryDir "${tid}_gpu_trace"

if (-not $XeniaExe) {
    $debugExe = "build\bin\Windows\Debug\xenia_canary.exe"
    $releaseExe = "build\bin\Windows\Release\xenia_canary.exe"
    switch ($Configuration) {
        "Debug" { $XeniaExe = $debugExe }
        "Release" { $XeniaExe = $releaseExe }
        default {
            if (Test-Path $debugExe) { $XeniaExe = $debugExe }
            else { $XeniaExe = $releaseExe }
        }
    }
}

New-Item -ItemType Directory -Force -Path $TelemetryDir | Out-Null
New-Item -ItemType Directory -Force -Path $gpuTrace | Out-Null

if ($FreshSession) {
    Remove-Item -Force -ErrorAction SilentlyContinue $stubLog, $xmaLog, $pcmLog, $crashLog
    Get-ChildItem -Path $gpuTrace -File -ErrorAction SilentlyContinue | Remove-Item -Force
}

if ($DurationSec -gt 0) {
    & "$PSScriptRoot\run_smoke_capture.ps1" `
        -TitleId $TitleId `
        -GamePath $GamePath `
        -XeniaExe $XeniaExe `
        -DurationSec $DurationSec `
        -TelemetryDir $TelemetryDir `
        -LogFile $crashLog `
        -Hid "xinput" `
        -TraceGpuPrefix $gpuTrace
    exit $LASTEXITCODE
}

# Quote game path so spaces (e.g. "Xbox 360") are not split by CreateProcess.
$exePath = (Resolve-Path $XeniaExe).Path
$gameQuoted = "`"$GamePath`""
$eventsLog = Join-Path $TelemetryDir "${tid}_events.jsonl"
$launchArgs = @(
    $gameQuoted,
    "--hid=xinput",
    "--log_preset=develop",
    "--log_file=$crashLog",
    "--obs_events_log=$eventsLog",
    "--kernel_stub_hit_log=$stubLog",
    "--apu_xma_divergence_log=$xmaLog",
    "--apu_pcm_hash_log=$pcmLog",
    "--trace_gpu_prefix=$gpuTrace",
    "--readback_resolve=fast",
    "--apu=xaudio2"
)
# In-game ground flicker workaround (compat #9). Opt-in only — can stall menu / add halo.
#   $env:PHOENIX_BF2_GFX_WORKAROUND = "1"
if ($env:PHOENIX_BF2_GFX_WORKAROUND -eq "1") {
    $launchArgs += @(
        "--depth_float24_convert_in_pixel_shader=true",
        "--gpu_allow_invalid_upload_range=true"
    )
}
if ($env:PHOENIX_DEBUG_PORT) {
    $launchArgs += "--phoenix_debug_port=$($env:PHOENIX_DEBUG_PORT)"
}

Write-Host "=== BF2 instrumented launch ($tid) ===" -ForegroundColor Cyan
Write-Host "Exe:        $exePath"
Write-Host "Play until crash. Telemetry:"
Write-Host "  crash log:  $crashLog"
Write-Host "  stubs:      $stubLog"
Write-Host "  events:     $eventsLog"
Write-Host "  pcm:        $pcmLog"
Write-Host "  gpu trace:  $gpuTrace"
Write-Host ""
if ($exePath -match "\\Release\\") {
    Write-Host "WARN: Release build cannot write F4 frame traces (NDEBUG)." -ForegroundColor Yellow
    Write-Host "      Use -Configuration Debug or build Debug xenia-app." -ForegroundColor Yellow
    Write-Host ""
}
Write-Host "GPU frame trace: press F4 once at broken menu/gfx; wait for .xtr in gpu_trace folder."
Write-Host "Do NOT start xenia_canary.exe from Explorer - use this script only."
Write-Host "After quit, run: .\tools\tier0\triage_gameplay_capture.ps1 -TitleId $TitleId"
Write-Host 'Verify launch: crash log must show hid=xinput and paths under telemetry\'
Write-Host ""

if (-not (Test-Path $exePath)) {
    Write-Host "Missing exe: $exePath" -ForegroundColor Red
    exit 1
}
if (-not (Test-Path $GamePath)) {
    Write-Host "Missing game path: $GamePath" -ForegroundColor Red
    exit 1
}

$proc = Start-Process -FilePath $exePath -ArgumentList $launchArgs -WorkingDirectory $repo -PassThru
Write-Host "PHOENIX_LAUNCH_PID=$($proc.Id)"
Write-Host "Xenia started (GUI) pid $($proc.Id). Shell returns immediately."
