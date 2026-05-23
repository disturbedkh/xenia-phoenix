# BF2 (454107DB) GPU proof-loop - instrumented launch or post-session gates.
# Run from repo root: powershell -File tools/tier0/bf2_gpu_session.ps1 -Launch
param(
    [string]$GamePath = "",
    [ValidateSet("Debug", "Release", "Auto")]
    [string]$Config = "Debug",
    [switch]$Launch,
    [switch]$PostOnly,
    [switch]$GfxWorkaround,
    [switch]$ReleaseMenu,
    [switch]$Triage
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

$tid = "454107DB"
$defaultIso = "D:\Xbox 360\Games\Battlefield 2 - Modern Combat (USA).iso"
if (-not $GamePath) { $GamePath = $defaultIso }

if (-not $env:PHOENIX_DEBUG_PORT) {
    $env:PHOENIX_DEBUG_PORT = "8765"
}

if ($PostOnly -and $Launch) {
    Write-Error "Use -Launch or -PostOnly, not both."
}

if (-not $PostOnly) {
    if (-not (Test-Path -LiteralPath $GamePath)) {
        Write-Error "ISO not found: $GamePath. Pass -GamePath with your ISO file path."
    }
    Write-Host "=== BF2 GPU session: instrumented launch ===" -ForegroundColor Cyan
    Write-Host "ISO: $GamePath"
    Write-Host "Probe: http://127.0.0.1:$($env:PHOENIX_DEBUG_PORT)"
    Write-Host ""
    $capArgs = @(
        "repro", "capture",
        "--title-id", $tid,
        "--game", $GamePath,
        "--config", $Config,
        "--launch",
        "--json"
    )
    if ($GfxWorkaround) { $capArgs += "--gfx-workaround" }
    if ($ReleaseMenu) { $capArgs += "--release-menu" }
    $launchJson = phoenixctl @capArgs 2>&1 | Out-String
    Write-Host $launchJson
    $launchResult = $launchJson | ConvertFrom-Json
    $guard = $launchResult.launch.launch_guard
    if ($guard -and -not $guard.ok) {
        Write-Host ""
        Write-Host "WARN: launch_guard reported issues (emulator may still be OK):" -ForegroundColor Yellow
        $guard.issues | ForEach-Object { Write-Host "  $_" }
        if ($launchResult.launch.pid) {
            Write-Host "  pid $($launchResult.launch.pid) - continue if Xenia is running." -ForegroundColor Yellow
        }
    }
    Write-Host ""
    Write-Host "NEXT:" -ForegroundColor Green
    Write-Host "  1. Play to in-game graphics repro (zone_01a ground flicker)."
    Write-Host "  2. Press F4 once at broken view (wait ~1s; do not double-tap)."
    Write-Host "  3. Quit emulator, then run:"
    Write-Host "       powershell -File tools/tier0/bf2_gpu_session.ps1 -PostOnly"
    if ($GfxWorkaround) {
        Write-Host "  (Gfx workaround ON - enable only before entering level, not at menu.)"
    }
    exit 0
}

Write-Host "=== BF2 GPU session: post gates ===" -ForegroundColor Cyan
$postArgs = @("repro", "capture", "--title-id", $tid, "--post-only", "--json")
$postJson = phoenixctl @postArgs 2>&1 | Out-String
Write-Host $postJson
$post = $postJson | ConvertFrom-Json

if ($Triage) {
    Write-Host ""
    Write-Host "=== Full triage report ===" -ForegroundColor Cyan
    phoenixctl triage --title-id $tid
}

$exit = 0
if (-not $post.ok) { $exit = 1 }
if ($post.fix_bar -and -not $post.fix_bar.trace_validate_pass) {
    if (-not $post.fix_bar.trace_missing) { $exit = 1 }
}
if ($post.launch_guard -and -not $post.launch_guard.ok) { $exit = 2 }

Write-Host ""
if ($post.fix_bar.trace_validate_pass) {
    Write-Host "PASS: trace validate. Next: probe snapshot at repro + metacache finding update." -ForegroundColor Green
} elseif ($post.fix_bar.trace_missing) {
    Write-Host "MISSING: F4 in-game trace. Re-launch with -Launch and capture at repro." -ForegroundColor Yellow
} else {
    Write-Host "FAIL: fix launch_guard or re-capture F4 trace." -ForegroundColor Red
}

exit $exit
