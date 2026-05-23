# Run xma2-diff against in-repo fixtures (Release build).
$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

$exe = "build\bin\Windows\Release\xma2-diff.exe"
if (-not (Test-Path $exe)) {
    $exe = "build\bin\Windows\xma2-diff.exe"
}
if (-not (Test-Path $exe)) {
    Write-Error "Build xma2-diff first (cmake -DXENIA_BUILD_MISC=ON)"
}

& $exe `
    --xma2_diff_fixtures_dir=tests/xma2_packets `
    --xma2_diff_report_out=tests/xma2_packets/golden/last_report.json `
    --xma2_diff_iters=1
exit $LASTEXITCODE
