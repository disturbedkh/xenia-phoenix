# Install git hooks for Tier 0 (optional)
#
# Run from repo root:
#   powershell -ExecutionPolicy Bypass -File tools/gpu_replay_ci/install-hooks.ps1

$ErrorActionPreference = "Stop"
$repo = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$hookDir = Join-Path (Join-Path $repo ".git") "hooks"
if (-not (Test-Path $hookDir)) {
  Write-Error "No .git/hooks directory — not a git checkout?"
}

$prePush = @'
#!/bin/sh
# Tier 0: optional GPU trace replay (requires Python + built trace-dump tools)
if command -v python3 >/dev/null 2>&1; then
  PY=python3
elif command -v python >/dev/null 2>&1; then
  PY=python
else
  exit 0
fi
if [ -d build/bin/Windows ] && [ -f tests/gpu_traces/*.xtr 2>/dev/null ]; then
  "$PY" tools/gpu_replay_ci/run.py --build-dir build --backend d3d12 --cross-path d3d12 || exit 1
fi
exit 0
'@

$prePushPath = Join-Path $hookDir "pre-push"
Set-Content -Path $prePushPath -Value $prePush -Encoding UTF8
Write-Host "Wrote $prePushPath (unix shell hook for Git Bash / MSYS)"
