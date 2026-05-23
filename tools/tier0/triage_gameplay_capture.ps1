# Post-capture triage for Tier 1 gameplay (stubs, XMA, PCM tail, crash log, patches).
param(
    [Parameter(Mandatory = $true)]
    [string]$TitleId,
    [string]$TelemetryDir = "telemetry"
)

$ErrorActionPreference = "Continue"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $repo

$tid = $TitleId.ToLower().Replace("0x", "")
$stubLog = Join-Path $TelemetryDir "${tid}_stubs.jsonl"
$xmaLog = Join-Path $TelemetryDir "${tid}_xma.jsonl"
$pcmLog = Join-Path $TelemetryDir "${tid}_pcm.jsonl"
$pcmBaseline = Join-Path $TelemetryDir "${tid}_pcm_baseline.jsonl"
$crashLog = Join-Path $TelemetryDir "${tid}_crash.log"
$reportPath = Join-Path $TelemetryDir "${tid}_triage_report.txt"
$stubSummary = Join-Path $TelemetryDir "${tid}_stub_summary.json"

$lines = New-Object System.Collections.Generic.List[string]
function Add-Line([string]$s) { $lines.Add($s) | Out-Null; Write-Host $s }

Add-Line "=== Gameplay triage report: $tid ==="
Add-Line "Generated: $(Get-Date -Format o)"
Add-Line ""

$exitCode = 0
$needsPlaythrough = $false

# --- Stubs ---
Add-Line "--- Kernel stubs ---"
if (Test-Path $stubLog) {
    python tools/tier0/aggregate_stub_hits.py $stubLog --out $stubSummary 2>&1 | ForEach-Object { Add-Line $_ }
    $summary = Get-Content $stubSummary -Raw | ConvertFrom-Json
    $gameStubs = @($summary.hits | Where-Object {
        $_.module -ne "phoenix" -and $_.export -notlike "*tier0_stub_log_smoke*"
    })
    if ($gameStubs.Count -eq 0) {
        Add-Line "STUB GATE: PASS (phoenix marker only or empty)"
    } else {
        Add-Line "STUB GATE: FAIL ($($gameStubs.Count) non-phoenix export(s))"
        $exitCode = 1
    }
} else {
    Add-Line "STUB GATE: SKIP (missing $stubLog)"
    $needsPlaythrough = $true
}

Add-Line ""

# --- XMA ---
Add-Line "--- XMA divergences ---"
if (Test-Path $xmaLog) {
    python tools/tier0/aggregate_xma_divergences.py $xmaLog --out (Join-Path $TelemetryDir "${tid}_xma_summary.json") 2>&1 | ForEach-Object { Add-Line $_ }
    $xmaLines = (Get-Content $xmaLog | Measure-Object -Line).Lines
    if ($xmaLines -eq 0) {
        Add-Line "XMA GATE: PASS (empty)"
    } else {
        Add-Line "XMA GATE: FAIL ($xmaLines lines)"
        $exitCode = 1
    }
} else {
    Add-Line "XMA GATE: PASS (no divergence file)"
}

Add-Line ""

# --- PCM full vs baseline ---
Add-Line "--- PCM vs baseline ---"
if (Test-Path $pcmLog) {
    if (Test-Path $pcmBaseline) {
        $cmpOut = python tools/tier0/compare_pcm_hash_log.py $pcmLog $pcmBaseline 2>&1
        $cmpLines = @($cmpOut | Measure-Object -Line).Lines
        if ($cmpLines -gt 25) {
            Add-Line "PCM FULL COMPARE: $cmpLines lines of drift (expected in long gameplay; see tail below)"
            $cmpOut | Select-Object -First 5 | ForEach-Object { Add-Line $_ }
            Add-Line "  ... truncated ..."
        } else {
            $cmpOut | ForEach-Object { Add-Line $_ }
        }
        if ($LASTEXITCODE -ne 0) {
            Add-Line "PCM FULL COMPARE: drift expected during gameplay; use tail analysis below"
        }
    } else {
        Add-Line "PCM BASELINE: missing $pcmBaseline (run compare_pcm_hash_log.py --update-baseline)"
    }
    Add-Line ""
    Add-Line "--- PCM boot (launch audio) ---"
    python tools/tier0/analyze_pcm_boot.py $pcmLog --boot-sec 120 2>&1 | ForEach-Object { Add-Line $_ }

    Add-Line ""
    Add-Line "--- PCM tail (crash-leading audio) ---"
    python tools/tier0/analyze_pcm_tail.py $pcmLog --tail-sec 120 2>&1 | ForEach-Object { Add-Line $_ }
} else {
    Add-Line "PCM: missing $pcmLog"
    $needsPlaythrough = $true
}

Add-Line ""

# --- Crash log ---
Add-Line "--- Host crash log ---"
if (Test-Path $crashLog) {
    python tools/tier0/triage_crash_log.py $crashLog 2>&1 | ForEach-Object { Add-Line $_ }
} else {
    Add-Line "CRASH LOG: MISSING ($crashLog)"
    Add-Line "  Run launch_bf2_triage.ps1 and play to crash with --log_file set"
    $needsPlaythrough = $true
}

Add-Line ""

# --- Patches ---
Add-Line "--- Patch debt (C+D) ---"
python tools/tier0/list_smoke_patches.py --title-id $TitleId 2>&1 | ForEach-Object { Add-Line $_ }

Add-Line ""

# --- GPU trace dir ---
$gpuTrace = Join-Path $TelemetryDir "${tid}_gpu_trace"
Add-Line "--- GPU frame trace ---"
if (Test-Path $gpuTrace) {
    $traceFiles = @(Get-ChildItem -Path $gpuTrace -Recurse -File -ErrorAction SilentlyContinue)
    Add-Line "GPU trace dir: $gpuTrace ($($traceFiles.Count) files)"
    if ($traceFiles.Count -eq 0) {
        Add-Line "  (empty - trigger frame trace when gfx degrades)"
    }
} else {
    Add-Line "GPU trace: no dir at $gpuTrace"
}

Add-Line ""

if ($needsPlaythrough) {
    Add-Line "STATUS: BLOCKED - need instrumented playthrough (launch_bf2_triage.ps1)"
    if ($exitCode -eq 0) { $exitCode = 1 }
} else {
    Add-Line "STATUS: Automated artifacts present; review buckets above for root-fix target"
}

Add-Line ""
Add-Line "Symptom-led priority: APU (PCM tail) then GPU (log + trace) for audio-cut-then-freeze"

$utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllLines($reportPath, $lines, $utf8NoBom)
Add-Line ""
Add-Line "Wrote $reportPath"

exit $exitCode
