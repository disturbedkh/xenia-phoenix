#Requires -Version 5.1
<#
.SYNOPSIS
  Run GPU trace RTV-vs-ROV replay (Phase 1.2 gate) from a VS Developer environment.

.PARAMETER BuildDir
  CMake build directory (default: build).

.PARAMETER AllowDrift
  Pass --allow-rtv-rov-drift to run.py (report only).
#>
param(
  [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
  [string]$BuildDir = "build",
  [switch]$AllowDrift
)

$ErrorActionPreference = "Stop"
Set-Location $SourceRoot

$vcvars = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
  $vcvars = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
}
if (-not (Test-Path $vcvars)) {
  Write-Error "vcvars64.bat not found - install VS 2022 C++ workload."
}

python tools/gpu_replay_ci/validate_traces.py --traces-dir tests/gpu_traces
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exe = Join-Path $SourceRoot "$BuildDir\bin\Windows\Release\xenia-gpu-d3d12-trace-dump.exe"
if (-not (Test-Path $exe)) {
  Write-Host "Building xenia-gpu-d3d12-trace-dump (Release)..."
  $buildCmd = "`"$vcvars`" && cmake --build `"$BuildDir`" --config Release --target xenia-gpu-d3d12-trace-dump -j 8"
  cmd /c $buildCmd
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$pyArgs = @(
  "tools/gpu_replay_ci/run.py",
  "--build-dir", $BuildDir,
  "--backend", "d3d12",
  "--cross-path", "d3d12"
)
if ($AllowDrift) { $pyArgs += "--allow-rtv-rov-drift" }

python @pyArgs
exit $LASTEXITCODE
