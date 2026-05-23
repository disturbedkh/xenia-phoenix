# GPU proof-loop door: instrumented launch, probe poll, post-session gates.
param(
    [Parameter(Mandatory = $true)]
    [string]$TitleId,
    [string]$GamePath = "",
    [ValidateSet("Debug", "Release", "Auto")]
    [string]$Config = "Debug",
    [string]$TelemetryDir = "telemetry",
    [switch]$Launch,
    [switch]$PostOnly,
    [switch]$GfxWorkaround,
    [switch]$ReleaseMenu,
    [int]$PollSec = 5,
    [int]$PollCount = 6,
    [string]$TracePath = "",
    [switch]$RunReplay
)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

$tid = $TitleId.ToUpper().Replace("0x", "")
$tidLower = $TitleId.ToLower().Replace("0x", "")
$gpuTraceDir = Join-Path $TelemetryDir "${tidLower}_gpu_trace"
$findingsPath = Join-Path (Split-Path $repo -Parent) "metacache\findings\games\${tidLower}.md"
if (-not (Test-Path $findingsPath)) {
    $findingsPath = "metacache/findings/games/${tidLower}.md (create from _template_game.md)"
}

$result = [ordered]@{
    ok           = $true
    title_id     = $tid
    repo         = $repo
    findings_path = $findingsPath
    launch       = $null
    probe_polls  = @()
    log_scan     = $null
    trace_validate = $null
    gpu_replay   = $null
    fix_bar      = [ordered]@{}
}

function Write-Info([string]$msg) {
    [Console]::Error.WriteLine($msg)
}

if ($GfxWorkaround) {
    $env:PHOENIX_BF2_GFX_WORKAROUND = "1"
} else {
    Remove-Item Env:PHOENIX_BF2_GFX_WORKAROUND -ErrorAction SilentlyContinue
}
if ($ReleaseMenu) {
    $Config = "Release"
}

if (-not $env:PHOENIX_DEBUG_PORT) {
    $env:PHOENIX_DEBUG_PORT = "8765"
}

if ($Launch -and -not $PostOnly) {
    Write-Info "=== GPU repro capture: launch ($tid) ==="
    $launchArgs = @(
        "launch", "triage",
        "--title-id", $tid,
        "--fresh",
        "--config", $Config
    )
    if ($GamePath) {
        $launchArgs += @("--game", $GamePath)
    }
    $launchJson = phoenixctl --json @launchArgs 2>&1 | Out-String
    try {
        $result.launch = $launchJson | ConvertFrom-Json
    } catch {
        $result.launch = @{ raw = $launchJson }
    }
    if ($result.launch.launch_guard -and -not $result.launch.launch_guard.ok) {
        Write-Info "WARN: launch_guard issues:"
        $result.launch.launch_guard.issues | ForEach-Object { Write-Info "  $_" }
    }
    Write-Info ""
    Write-Info "Play to graphics repro. Press F4 once at broken view (in-game, not loading screen)."
    Write-Info "Then re-run with -PostOnly or wait for probe poll below."
    Write-Info ""
    Start-Sleep -Seconds 8
}

if (-not $PostOnly -and $PollCount -gt 0) {
    Write-Info "=== Probe poll (${PollSec}s x $PollCount) ==="
    for ($i = 1; $i -le $PollCount; $i++) {
        Write-Info "--- poll $i / $PollCount ---"
        try {
            $snap = phoenixctl --json probe snapshot 2>&1 | Out-String | ConvertFrom-Json
            $result.probe_polls += $snap
        } catch {
            $result.probe_polls += @{ error = $_.Exception.Message }
            $result.ok = $false
        }
        if ($i -lt $PollCount) { Start-Sleep -Seconds $PollSec }
    }
}

Write-Info "=== Post-session gates ==="

try {
    $sumArgs = @("log", "summarize", "--title-id", $tid, "--write-summary")
    $sumJson = phoenixctl --json @sumArgs 2>&1 | Out-String | ConvertFrom-Json
    $result.obs_summary = $sumJson
    $summaryPath = Join-Path $TelemetryDir "${tidLower}_obs_summary.json"
    if (Test-Path $summaryPath) {
        Write-Info "obs_summary: $summaryPath"
        if ($sumJson.classification) {
            $cls = $sumJson.classification
            Write-Info "  classification: $($cls.symptom_bucket) findings=$($cls.finding_ids -join ',')"
        }
    }
} catch {
    $result.obs_summary = @{ error = $_.Exception.Message }
}

try {
    $guardRaw = phoenixctl --json guard check --title-id $tid 2>&1 | Out-String
    $result.launch_guard = $guardRaw | ConvertFrom-Json
    if ($result.launch_guard -and -not $result.launch_guard.ok) {
        $result.ok = $false
        Write-Info "FAIL: launch_guard (mis-instrumented session):"
        $result.launch_guard.issues | ForEach-Object { Write-Info "  $_" }
        Write-Info "  Fix: phoenixctl launch triage --title-id $tid --game <iso> --fresh"
    }
} catch {
    $result.launch_guard = @{ error = $_.Exception.Message }
}

try {
    $result.log_scan = phoenixctl --json log scan --title-id $tid 2>&1 | Out-String | ConvertFrom-Json
} catch {
    $result.log_scan = @{ error = $_.Exception.Message }
}

$xtr = $TracePath
if (-not $xtr -and (Test-Path $gpuTraceDir)) {
    $latest = Get-ChildItem -Path $gpuTraceDir -Filter "*.xtr" -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($latest) { $xtr = $latest.FullName }
}

if ($xtr -and (Test-Path $xtr)) {
    Write-Info "Validating trace: $xtr"
    try {
        $tvJson = phoenixctl --json trace validate $xtr 2>&1 | Out-String | ConvertFrom-Json
        $result.trace_validate = @{ path = $xtr; result = $tvJson }
        $result.fix_bar.trace_validate_pass = ($tvJson.exit_code -eq 0)
    } catch {
        $result.trace_validate = @{ path = $xtr; error = $_.Exception.Message }
        $result.fix_bar.trace_validate_pass = $false
    }
} else {
    Write-Info "No .xtr found under $gpuTraceDir (press F4 at repro)"
    $result.fix_bar.trace_validate_pass = $false
    $result.fix_bar.trace_missing = $true
}

$lastSnap = $result.probe_polls[-1]
if ($lastSnap -and $lastSnap.snapshot -and $lastSnap.snapshot.body) {
    $body = $lastSnap.snapshot.body
    if ($body.gpu) {
        $result.fix_bar.last_gpu_counters = $body.gpu
    }
}

$result.fix_bar.upload_range_errors = $result.log_scan.upload_range_errors
$result.fix_bar.launch_guard_ok = $false
if ($result.launch_guard -and $result.launch_guard.ok) {
    $result.fix_bar.launch_guard_ok = $true
}
$result.fix_bar.reminder = @(
    "Update findings: $findingsPath"
    "Native fix bar: metacache/dev/gpu_fix_bar.md"
    "Link promotes_to U-GPU-* in findings/universal/gpu_edram.md"
)
if (-not $result.fix_bar.launch_guard_ok) {
    $result.fix_bar.reminder = @(
        "BLOCKED: use tools/tier0/bf2_gpu_session.ps1 -Launch (not raw xenia_canary.exe)"
    ) + $result.fix_bar.reminder
}

if ($RunReplay -and $xtr) {
    Write-Info "WARN: run_gpu_replay uses synthetic corpus; retail .xtr may not be in CI."
    try {
        $replayOut = & "$PSScriptRoot\run_gpu_replay.ps1" 2>&1 | Out-String
        $result.gpu_replay = @{ output = $replayOut; note = "corpus replay not per-file unless added to CORPUS" }
    } catch {
        $result.gpu_replay = @{ error = $_.Exception.Message }
    }
}

Write-Info ""
Write-Info "=== gpu_fix_bar hints ==="
Write-Info "  trace validate: $($result.fix_bar.trace_validate_pass)"
if ($result.fix_bar.last_gpu_counters) {
    $g = $result.fix_bar.last_gpu_counters
    Write-Info "  ownership_change_count: $($g.ownership_change_count)"
    Write-Info "  edram_transfer_count: $($g.edram_transfer_count)"
    Write-Info "  host_depth_store_count: $($g.host_depth_store_count)"
    Write-Info "  host_depth_transfer_mismatch_count: $($g.host_depth_transfer_mismatch_count)"
    Write-Info "  upload_range_error_count: $($g.upload_range_error_count)"
}
Write-Info "  log upload_range_errors: $($result.log_scan.upload_range_errors)"

# JSON only on stdout for phoenixctl / MCP
$result | ConvertTo-Json -Depth 8 -Compress:$false
if (-not $result.ok) { exit 1 }
exit 0
