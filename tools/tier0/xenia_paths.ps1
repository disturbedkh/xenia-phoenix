# Shared Phoenix build path resolver (wraps tools/build/xenia_paths.py).
function Get-XeniaRepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
}

function Get-XeniaPathsPy {
    return Join-Path (Get-XeniaRepoRoot) "tools/build/xenia_paths.py"
}

function Get-XeniaBinDir {
    param(
        [string]$Config = "Release",
        [string]$TargetArch = $null,
        [string]$Os = $null
    )
    $args = @("bin", "--config", $Config)
    if ($TargetArch) { $args += @("--target-arch", $TargetArch) }
    if ($Os) { $args += @("--os", $Os) }
    & python (Get-XeniaPathsPy) @args
}

function Get-XeniaBuildDir {
    param(
        [string]$TargetArch = $null,
        [string]$Os = $null
    )
    $args = @("build-dir")
    if ($TargetArch) { $args += @("--target-arch", $TargetArch) }
    if ($Os) { $args += @("--os", $Os) }
    & python (Get-XeniaPathsPy) @args
}
