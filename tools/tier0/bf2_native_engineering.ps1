# BF2 U-GPU-001 engineering harness: corpus install, build, validate, replay, baseline.
# Run from xenia-phoenix-src:
#   powershell -File tools/tier0/bf2_native_engineering.ps1 -All
param(
    [switch]$InstallCorpus,
    [switch]$Build,
    [switch]$ValidateCorpus,
    [switch]$Replay,
    [switch]$WriteBaseline,
    [switch]$All,
    [string]$BuildDir = "build",
    [switch]$AllowDrift,
    [switch]$Include7687
)

$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
Set-Location $repo

$tidLower = "454107db"
$telemetryGpu = Join-Path $repo "telemetry\${tidLower}_gpu_trace"
$corpusDir = Join-Path $repo "tests\gpu_traces"
$metacacheDev = Join-Path (Split-Path $repo -Parent) "metacache\dev"

function Get-VcVars64 {
    $candidates = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    )
    foreach ($p in $candidates) {
        if (Test-Path -LiteralPath $p) { return $p }
    }
    throw "vcvars64.bat not found - install VS 2022 C++ workload."
}

function Invoke-CmdBuild {
    param([string]$Config, [string]$Target)
    $vcvars = Get-VcVars64
    $cmd = "`"$vcvars`" && cmake --build `"$BuildDir`" --config $Config --target $Target -j 8"
    Write-Host ">> build $Target ($Config)" -ForegroundColor Cyan
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $Target ($Config)" }
}

function Install-CorpusSymlink {
    param([string]$TraceName)
    $src = Join-Path $telemetryGpu $TraceName
    $dst = Join-Path $corpusDir $TraceName
    if (-not (Test-Path -LiteralPath $src)) {
        Write-Warning "Missing trace (skip symlink): $src"
        return $false
    }
    if (Test-Path -LiteralPath $dst) {
        $item = Get-Item -LiteralPath $dst -Force
        if ($item.Length -eq (Get-Item -LiteralPath $src).Length) {
            Write-Host "Corpus link OK: $TraceName"
            return $true
        }
        Remove-Item -LiteralPath $dst -Force
    }
    try {
        New-Item -ItemType SymbolicLink -Path $dst -Target $src -ErrorAction Stop | Out-Null
        Write-Host "Symlinked: $dst -> $src"
        return $true
    } catch {
        cmd /c "mklink /H `"$dst`" `"$src`""
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Link failed for $TraceName (need symlink admin or same-volume hardlink): $_"
            return $false
        }
        Write-Host "Hard-linked: $dst -> $src"
        return $true
    }
}

function Ensure-SyntheticCorpus {
    $any = @(Get-ChildItem -Path $corpusDir -Filter "*.xtr" -ErrorAction SilentlyContinue)
    if ($any.Count -gt 0) { return }
    Write-Host "No .xtr in corpus; generating synthetic phase12 fixtures..." -ForegroundColor Yellow
    python tools/gpu_replay_ci/gen_phase12_fixture_xtr.py
    if ($LASTEXITCODE -ne 0) { throw "gen_phase12_fixture_xtr.py failed" }
}

if ($All) {
    $InstallCorpus = $true
    $Build = $true
    $ValidateCorpus = $true
    $Replay = $true
    $WriteBaseline = $true
}

if ($InstallCorpus) {
    Write-Host "=== InstallCorpus ===" -ForegroundColor Cyan
    if (-not (Test-Path -LiteralPath $corpusDir)) {
        New-Item -ItemType Directory -Path $corpusDir | Out-Null
    }
    $null = Install-CorpusSymlink "454107DB_6004.xtr"
    if ($Include7687) {
        $null = Install-CorpusSymlink "454107DB_7687.xtr"
    }
    Ensure-SyntheticCorpus
}

if ($Build) {
    Write-Host "=== Build (Debug xenia_canary + Release trace-dump) ===" -ForegroundColor Cyan
    Invoke-CmdBuild -Config Debug -Target xenia-app
    Invoke-CmdBuild -Config Release -Target xenia-gpu-d3d12-trace-dump
}

if ($ValidateCorpus) {
    Write-Host "=== ValidateCorpus ===" -ForegroundColor Cyan
    Ensure-SyntheticCorpus
    python tools/gpu_replay_ci/validate_traces.py --traces-dir $corpusDir
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $bf2 = @(Get-ChildItem -Path $corpusDir -Filter "454107DB_*.xtr" -ErrorAction SilentlyContinue)
    if ($bf2.Count -eq 0) {
        Write-Warning "No 454107DB_*.xtr in corpus - run InstallCorpus after F4 capture."
    } else {
        python tools/gpu_replay_ci/validate_traces.py @($bf2.FullName)
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}

if ($Replay) {
    Write-Host "=== Replay ===" -ForegroundColor Cyan
    $replayArgs = @("-File", (Join-Path $PSScriptRoot "run_gpu_replay.ps1"), "-BuildDir", $BuildDir)
    if ($AllowDrift) { $replayArgs += "-AllowDrift" }
    & powershell @replayArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if ($WriteBaseline) {
    Write-Host "=== WriteBaseline ===" -ForegroundColor Cyan
    if (-not (Test-Path -LiteralPath $metacacheDev)) {
        New-Item -ItemType Directory -Path $metacacheDev | Out-Null
    }
    $baseline = [ordered]@{
        trace                               = "454107DB_7687.xtr"
        session_note                        = "Frozen before U-GPU-001 fix (in-game poll at ground repro)"
        readback_resolve                    = "none"
        host_depth_transfer_mismatch_count  = 263
        ownership_change_count              = 1450
        edram_transfer_count              = 1450
        host_depth_store_count              = 131
        upload_range_error_count            = 5
        post_fix_acceptance                 = [ordered]@{
            host_depth_transfer_mismatch_count = 0
            requires_visual_flicker_gone         = $true
            no_boot_workaround_cvars             = $true
        }
    }
    $outPath = Join-Path $metacacheDev "bf2_probe_baseline_7687.json"
    $baseline | ConvertTo-Json -Depth 6 | Set-Content -Path $outPath -Encoding UTF8
    Write-Host "Wrote $outPath"

    $workaround = [ordered]@{
        trace      = "454107DB_7687.xtr"
        session_note = "Optional A/B with PHOENIX_BF2_GFX_WORKAROUND=1 (masks U-GPU-001, halo risk)"
        env        = "PHOENIX_BF2_GFX_WORKAROUND=1"
        counters   = "Fill after optional user GfxWorkaround session"
    }
    $workaroundPath = Join-Path $metacacheDev "bf2_probe_baseline_workaround.json"
    $workaround | ConvertTo-Json -Depth 4 | Set-Content -Path $workaroundPath -Encoding UTF8
    Write-Host "Wrote $workaroundPath (template for optional A/B)"
}

Write-Host "Done." -ForegroundColor Green
