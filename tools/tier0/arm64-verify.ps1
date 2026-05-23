# Post-build verification for Windows ARM64 (WoA) builds.
param(
    [string]$BuildDir = "build-arm64",
    [string]$Config = "Release",
    [switch]$SkipCpuTests
)

$ErrorActionPreference = "Stop"
$bin = Join-Path $BuildDir "bin\Windows\$Config"
$exe = Join-Path $bin "xenia_canary.exe"
$cpuTests = Join-Path $bin "xenia-cpu-tests.exe"

if (-not (Test-Path $exe)) {
    Write-Error "Missing $exe — run: python xenia-build.py build --target-arch arm64 --config=release --target=xenia-app --build-tests"
}

$len = (Get-Item $exe).Length
if ($len -le 100000) {
    Write-Error "xenia_canary.exe too small ($len bytes); build may have failed."
}
Write-Host "Smoke: $exe --help"
& $exe --help | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Error "Smoke --help failed with exit $LASTEXITCODE"
}

if (-not $SkipCpuTests) {
    if (-not (Test-Path $cpuTests)) {
        Write-Warning "Skipping cpu-tests: $cpuTests not found (rebuild with --build-tests)"
    } else {
        Write-Host "CPU tests: $cpuTests"
        Push-Location $bin
        try {
            & ".\xenia-cpu-tests.exe"
            if ($LASTEXITCODE -ne 0) {
                Write-Error "xenia-cpu-tests failed with exit $LASTEXITCODE"
            }
        } finally {
            Pop-Location
        }
    }
}

Write-Host "arm64-verify: OK"
