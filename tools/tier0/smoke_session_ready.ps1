# Pre-flight for Tier 1 smoke capture. Run from xenia-phoenix-src.
param(
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$repo = $PSScriptRoot | Split-Path | Split-Path
Set-Location $repo

$exe = Join-Path $repo "build\bin\Windows\$Config\xenia_canary.exe"
$fuzz = Join-Path $repo "build\bin\Windows\$Config\vmx128-fuzz.exe"

Write-Host "=== Smoke session pre-flight ===" -ForegroundColor Cyan
Write-Host "Repo: $repo"

$ok = $true
if (Test-Path $exe) {
    $len = (Get-Item $exe).Length
    Write-Host "[OK] xenia_canary.exe ($([math]::Round($len/1MB, 1)) MB)" -ForegroundColor Green
} else {
    Write-Host "[MISSING] $exe" -ForegroundColor Red
    Write-Host "  Build: python xenia-build.py build --config=$Config --target=xenia-app"
    $ok = $false
}

if (Test-Path $fuzz) {
    Write-Host "[OK] vmx128-fuzz.exe (Tier 0 CPU gate)" -ForegroundColor Green
} else {
    Write-Host "[WARN] vmx128-fuzz.exe not found (optional for smoke)" -ForegroundColor Yellow
}

New-Item -ItemType Directory -Force -Path telemetry | Out-Null
Write-Host "[OK] telemetry/ directory"

if ($env:PHOENIX_DEBUG_PORT) {
    $p = [int]$env:PHOENIX_DEBUG_PORT
    try {
        $h = Invoke-WebRequest -Uri "http://127.0.0.1:$p/health" -UseBasicParsing -TimeoutSec 2
        if ($h.StatusCode -eq 200) {
            Write-Host "[OK] phoenix_probe on port $p" -ForegroundColor Green
        } else {
            Write-Host "[WARN] probe port $p returned $($h.StatusCode)" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "[WARN] PHOENIX_DEBUG_PORT=$p but probe not reachable" -ForegroundColor Yellow
    }
}

$scripts = @(
    "tools/tier0/run_smoke_capture.ps1",
    "tools/tier0/list_smoke_patches.py",
    "tools/tier0/aggregate_stub_hits.py"
)
foreach ($s in $scripts) {
    if (-not (Test-Path $s)) { Write-Host "[MISSING] $s" -ForegroundColor Red; $ok = $false }
}
if ($ok) { Write-Host "[OK] capture scripts" -ForegroundColor Green }

Write-Host ""
Write-Host "Recommended title IDs (if you own them):" -ForegroundColor Cyan
Write-Host "  4D5307D1  Project Gotham Racing 3   (pipeline shakedown)"
Write-Host "  4D5307EA  Forza Motorsport 2        (kernel stubs)"
Write-Host "  4D5309B1  Halo CE Anniversary       (XMA/audio)"
Write-Host "  58410955  Banjo-Tooie               (GPU)"
Write-Host "  4D53082D  Gears of War 2            (GPU)"
Write-Host ""
Write-Host "When ready, tell the agent: title ID + path to default.xex"
Write-Host "Capture command template:"
Write-Host @"
  .\tools\tier0\run_smoke_capture.ps1 `
    -TitleId <ID> `
    -GamePath "<path\to\default.xex>" `
    -XeniaExe build\bin\Windows\$Config\xenia_canary.exe `
    -DurationSec 300
"@

if (-not $ok) { exit 1 }
