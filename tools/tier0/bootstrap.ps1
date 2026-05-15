#Requires -Version 5.1
<#
.SYNOPSIS
  Configure and build Xenia Canary for Tier 0 (tests + misc tools).

.PARAMETER SourceRoot
  Path to xenia-canary repo root (folder containing CMakeLists.txt).

.PARAMETER Preset
  CMake configure preset: default (Ninja Multi-Config) or vs.

.PARAMETER Configuration
  Release, Debug, or Checked.

.PARAMETER BuildTests
  Pass -DXENIA_BUILD_TESTS=ON

.PARAMETER BuildMisc
  Pass -DXENIA_BUILD_MISC=ON (trace dump tools, vmx128-fuzz, etc.)

.PARAMETER RunCpuTests
  After build, run ctest or xenia-cpu-tests for the selected configuration.
#>
param(
  [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
  [ValidateSet("default", "vs")]
  [string]$Preset = "vs",
  [ValidateSet("Release", "Debug", "Checked")]
  [string]$Configuration = "Release",
  [switch]$BuildTests = $true,
  [switch]$BuildMisc = $true,
  [switch]$RunCpuTests = $false
)

$ErrorActionPreference = "Stop"
Set-Location $SourceRoot

Write-Host "SourceRoot: $SourceRoot"

if (-not (Test-Path (Join-Path $SourceRoot ".gitmodules"))) {
  Write-Warning "No .gitmodules at repo root; check path."
}

Write-Host "Updating submodules..."
git submodule update --init --recursive

$extra = @()
if ($BuildTests) { $extra += "-DXENIA_BUILD_TESTS=ON" }
if ($BuildMisc) { $extra += "-DXENIA_BUILD_MISC=ON" }

Write-Host "CMake configure preset=$Preset $extra"
& cmake --preset $Preset @extra
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "CMake build config=$Configuration"
& cmake --build build --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$binDir = Join-Path $SourceRoot "build\bin\Windows"
Write-Host "Binaries: $binDir"

if ($RunCpuTests) {
  $cpuTests = Join-Path $binDir "xenia-cpu-tests.exe"
  if (Test-Path $cpuTests) {
    Write-Host "Running $cpuTests ..."
    & $cpuTests
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  } else {
    Write-Warning "xenia-cpu-tests.exe not found (build tests enabled?)"
  }
}

Write-Host "Done."
