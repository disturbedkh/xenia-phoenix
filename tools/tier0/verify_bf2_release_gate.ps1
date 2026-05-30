# Post-build checklist for BF2 Release long-session verification (Track 3).
param(
    [string]$TitleId = "454107DB",
    [string]$GamePath = "",
    [string]$TelemetryDir = "telemetry"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

. (Join-Path $PSScriptRoot "xenia_paths.ps1")
$releaseExe = Join-Path (Get-XeniaBinDir -Config Release) "xenia_canary.exe"
if (-not (Test-Path $releaseExe)) {
    Write-Host "Build Release first: python xenia-build.py build --config=release --target=xenia-app" -ForegroundColor Red
    exit 1
}

Write-Host "=== BF2 Release verification gate ($TitleId) ===" -ForegroundColor Cyan
Write-Host "Release exe: $releaseExe"
Write-Host ""
Write-Host "1. Run instrumented launch (Release, ~20+ min to prior crash point):"
if ($GamePath) {
    Write-Host "   .\tools\tier0\launch_bf2_triage.ps1 -Configuration Release -FreshSession -GamePath `"$GamePath`""
} else {
    Write-Host "   .\tools\tier0\launch_bf2_triage.ps1 -Configuration Release -FreshSession -GamePath <iso>"
}
Write-Host "2. After quit:"
Write-Host "   .\tools\tier0\triage_gameplay_capture.ps1 -TitleId $TitleId"
Write-Host "3. Pass: no 'Skipping draw - pipeline not ready' in crash log;"
Write-Host "         load/gfx improved vs prior; stubs/XMA gates green."
Write-Host ""

$triage = Join-Path $TelemetryDir ($TitleId.ToLower().Replace("0x", "") + "_triage_report.txt")
if (Test-Path $triage) {
    Write-Host "Latest triage report:" -ForegroundColor Yellow
    Get-Content $triage | Select-Object -First 25
}
