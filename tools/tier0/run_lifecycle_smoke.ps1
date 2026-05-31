# Lifecycle smoke: manual gate for return-to-UI (launch title A, close, launch title B).
# Requires a legal game path. Example:
#   .\tools\tier0\run_lifecycle_smoke.ps1 -GameA "D:\Games\title1.iso" -GameB "D:\Games\title2.iso"

param(
  [Parameter(Mandatory = $true)]
  [string] $GameA,
  [Parameter(Mandatory = $true)]
  [string] $GameB,
  [string] $XeniaExe = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "xenia_paths.ps1")
if (-not $XeniaExe) {
  $XeniaExe = Join-Path (Get-XeniaBinDir -Config Debug) "xenia_canary.exe"
}
if (-not (Test-Path $XeniaExe)) {
  throw "xenia_canary.exe not found at $XeniaExe; build xenia-app first."
}

Write-Host "Lifecycle smoke (manual):"
Write-Host "  1) Emulator opens with launcher."
Write-Host "  2) Launch GameA from File > Open or drag-drop: $GameA"
Write-Host "  3) File > Close (DEBUG builds) or exit title; confirm launcher returns."
Write-Host "  4) Launch GameB: $GameB"
Write-Host "  5) Exit app; confirm clean shutdown (no quick_exit) in xenia.log."
Write-Host ""
Write-Host "Starting xenia (first title via --target for automation of step 2 only)..."
& $XeniaExe $GameA
