# Phoenix Linux build via Docker (Ubuntu 24.04).
param(
    [ValidateSet("release", "debug", "checked", "all")]
    [string]$Config = "release",
    [string]$Image = "ubuntu:24.04",
    [switch]$SkipVerify,
    [int]$DockerWaitSeconds = 180
)

$ErrorActionPreference = "Stop"
$Src = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

function Ensure-Docker {
    docker info 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) { return }

    $dockerDesktop = "${env:ProgramFiles}\Docker\Docker\Docker Desktop.exe"
    if (Test-Path $dockerDesktop) {
        Write-Host "Starting Docker Desktop..."
        Start-Process $dockerDesktop
    }

    $deadline = (Get-Date).AddSeconds($DockerWaitSeconds)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 5
        docker info 2>$null | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Docker daemon is ready."
            return
        }
    }
    throw "Docker daemon not available after ${DockerWaitSeconds}s. Start Docker Desktop manually."
}

function Invoke-LinuxBuild {
    param([string]$BuildConfig)
    docker run --rm `
        -e $(if ($SkipVerify) { "SKIP_VERIFY=1" } else { "SKIP_VERIFY=0" }) `
        -v "${Src}:/src:rw" `
        -w /src `
        $Image `
        bash -lc "sed -i 's/\r$//' tools/docker/linux-build.sh tools/docker/linux-verify.sh && bash tools/docker/linux-build.sh $BuildConfig"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Docker build failed for config: $BuildConfig" -ForegroundColor Red
        return $LASTEXITCODE
    }
    $logName = "linux_build_$BuildConfig.log"
    $logSrc = Join-Path $Src $logName
    if (Test-Path $logSrc) {
        Write-Host "Log: $logSrc"
    }
    return 0
}

Ensure-Docker

$configs = if ($Config -eq "all") { @("release", "debug", "checked") } else { @($Config) }

foreach ($c in $configs) {
    Write-Host "=== Linux build: $c ===" -ForegroundColor Cyan
    $code = Invoke-LinuxBuild -BuildConfig $c
    if ($code -ne 0) {
        exit $code
    }
}

exit 0
