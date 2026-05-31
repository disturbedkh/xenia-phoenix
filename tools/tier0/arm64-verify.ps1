# Post-build verification for Windows ARM64 (WoA) builds.
param(
    [string]$Config = "Release",
    [switch]$SkipCpuTests,
    [switch]$ForceRunSmoke
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "../..")
$pathsPy = Join-Path $repoRoot "tools/build/xenia_paths.py"
$bin = & python $pathsPy bin --config $Config --target-arch arm64
$exe = Join-Path $bin "xenia_canary.exe"
$cpuTests = Join-Path $bin "xenia-cpu-tests.exe"

function Get-PeMachineType {
    param([string]$Path)
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x40) { return $null }
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
    if ($peOffset -lt 0 -or ($peOffset + 6) -ge $bytes.Length) { return $null }
    return [BitConverter]::ToUInt16($bytes, $peOffset + 4)
}

if (-not (Test-Path $exe)) {
    Write-Error "Missing $exe — run: python xenia-build.py build --target-arch arm64 --config=release --target=xenia-app --build-tests"
}

$len = (Get-Item $exe).Length
if ($len -le 100000) {
    Write-Error "xenia_canary.exe too small ($len bytes); build may have failed."
}

$machine = Get-PeMachineType $exe
# IMAGE_FILE_MACHINE_ARM64
if ($machine -ne 0xAA64) {
    Write-Error "Expected ARM64 PE (machine 0xAA64), got 0x{0:X4} for $exe" -f $machine
}
Write-Host "PE machine: ARM64 (0xAA64), size $len bytes"

$hostIsArm64 = @('ARM64', 'aarch64') -contains $env:PROCESSOR_ARCHITECTURE
if (-not $hostIsArm64 -and -not $ForceRunSmoke) {
    Write-Host "Cross-compile host ($env:PROCESSOR_ARCHITECTURE): skipping --help and cpu-tests (PE check only)."
    if (-not $SkipCpuTests) {
        Write-Host "Hint: run on WoA hardware or use -ForceRunSmoke if ARM64 emulation is available."
    }
    Write-Host "arm64-verify: OK"
    exit 0
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
