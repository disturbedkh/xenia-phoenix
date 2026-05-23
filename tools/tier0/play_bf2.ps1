# Normal BF2 play launch (Xbox controller + fast readback). Use instead of double-clicking the exe.
param(
    [Parameter(Mandatory = $true)]
    [string]$GamePath,
    [ValidateSet("Debug", "Release", "Auto")]
    [string]$Configuration = "Auto"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
& (Join-Path $PSScriptRoot "launch_bf2_triage.ps1") `
    -GamePath $GamePath `
    -Configuration $Configuration
