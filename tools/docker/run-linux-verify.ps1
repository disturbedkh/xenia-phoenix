# Resume Linux verify without a full rebuild (needs existing Linux build tree).
param(
    [ValidateSet("release", "debug", "checked")]
    [string]$Config = "release",
    [ValidateSet("post-build", "smoke-only", "cpu-tests-only")]
    [string]$Mode = "cpu-tests-only",
    [string]$Image = "phoenix-linux-build:24.04"
)

$ErrorActionPreference = "Stop"
$Src = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

$usePrebuilt = $false
try {
    docker image inspect $Image 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) { $usePrebuilt = $true }
} catch { }
$baseImage = if ($usePrebuilt) { $Image } else { "ubuntu:24.04" }

Write-Host "Linux verify ($Mode) using image: $baseImage" -ForegroundColor Cyan

docker run --rm -t `
    -v "${Src}:/src:rw" `
    -w /src `
    $baseImage `
    bash -lc "sed -i 's/\r$//' tools/docker/linux-build.sh tools/docker/linux-verify.sh xenia-build.py && bash tools/docker/linux-build.sh $Config $Mode"

exit $LASTEXITCODE
