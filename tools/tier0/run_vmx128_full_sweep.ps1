# Phase 1.1: local 1M/opcode vmx128-fuzz sweep + markdown stub from JSON.
# Expects vmx128-fuzz.exe under Build/Windows/x64/Release after `cmake --preset vs` + Release build.

$ErrorActionPreference = "Stop"

$XeniaSrc = Split-Path (Split-Path $PSScriptRoot)
. (Join-Path $PSScriptRoot "xenia_paths.ps1")
$FuzzExe = Join-Path (Get-XeniaBinDir -Config Release) "vmx128-fuzz.exe"
if (-not (Test-Path $FuzzExe)) {
  Write-Error "vmx128-fuzz.exe not found at $FuzzExe (configure + build Release first)."
}

# Reproducible sign-off seed (0xDEADBEEF).
$Seed = [uint32]3735928559
$JsonOut = Join-Path $XeniaSrc "docs/vmx128_fuzz_report.json"
$MdOut = Join-Path $XeniaSrc "docs/vmx128_fuzz_report.md"

Write-Host "vmx128 pass 1: 1M/op, rm=rn, seed=$Seed"
& $FuzzExe --vmx128_fuzz_iters=1000000 --vmx128_fuzz_seed=$Seed --vmx128_fuzz_rm_pass=rn --vmx128_fuzz_report_out=$JsonOut
$pass1 = $LASTEXITCODE
if ($pass1 -ne 0) {
  Write-Error "vmx128-fuzz rn pass failed (exit $pass1). Fix mismatches before rm=all pass."
}

Write-Host "vmx128 pass 2: 1M/op, rm=all, seed=$Seed"
& $FuzzExe --vmx128_fuzz_iters=1000000 --vmx128_fuzz_seed=$Seed --vmx128_fuzz_rm_pass=all --vmx128_fuzz_report_out=$JsonOut
$pass2 = $LASTEXITCODE
if ($pass2 -ne 0) { exit $pass2 }

$json = Get-Content -Raw $JsonOut
if (-not $json) {
  Write-Error "Missing report JSON after successful passes: $JsonOut"
}
$md = @()
$md += "# vmx128-fuzz report (local sweep)"
$md += ""
$md += "- **Generated**: $([DateTime]::UtcNow.ToString('o')) (UTC)"
$md += "- **Seed**: $Seed"
$md += "- **JSON**: docs/vmx128_fuzz_report.json"
$md += "- **Passes**: rn then rm=all (both must be zero mismatches)"
$md += ""
$md += "## Raw summary"
$md += ""
$md += $json.TrimEnd()
$md += ""
$md -join "`n" | Set-Content -Encoding utf8 $MdOut
Write-Host "Wrote $MdOut"
exit 0
