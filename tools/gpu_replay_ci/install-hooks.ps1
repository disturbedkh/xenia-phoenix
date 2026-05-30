# Install git hooks for Phoenix (optional local mirror of the cheap PR gates).
#
# Run from repo root:
#   powershell -ExecutionPolicy Bypass -File tools/gpu_replay_ci/install-hooks.ps1
#
# The generated pre-push hook runs the fast, deterministic gates that block PRs:
#   1. build-path guard  (tools/build/check_build_paths.py) - hard gate
#   2. clang-format lint of changed files (xenia-build.py lint) - if clang-format present
#   3. GPU trace replay - only when a Windows x64 build + .xtr traces exist
# Bypass any time with:  git push --no-verify

$ErrorActionPreference = "Stop"
$repo = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$hookDir = Join-Path (Join-Path $repo ".git") "hooks"
if (-not (Test-Path $hookDir)) {
  Write-Error "No .git/hooks directory - not a git checkout?"
}

$prePush = @'
#!/bin/sh
# Phoenix pre-push: local mirror of the cheap PR-blocking gates.
# Bypass with: git push --no-verify
if command -v python3 >/dev/null 2>&1; then
  PY=python3
elif command -v python >/dev/null 2>&1; then
  PY=python
else
  echo "pre-push: python not found; skipping Phoenix checks." >&2
  exit 0
fi

# 1. Build-path guard (pure Python, always runs; hard gate).
"$PY" tools/build/check_build_paths.py || {
  echo "pre-push: build-path guard failed (python tools/build/check_build_paths.py)." >&2
  exit 1
}

# 2. clang-format lint of changed files (only when clang-format is available).
if command -v clang-format >/dev/null 2>&1 || command -v git-clang-format >/dev/null 2>&1; then
  "$PY" xenia-build.py lint || {
    echo "pre-push: lint failed. Fix with: python xenia-build.py format" >&2
    exit 1
  }
else
  echo "pre-push: clang-format not found; skipping lint." >&2
fi

# 3. Optional GPU trace replay when a Windows x64 build + traces exist.
if [ -d Build/Windows/x64/Release ] && ls tests/gpu_traces/*.xtr >/dev/null 2>&1; then
  "$PY" tools/gpu_replay_ci/run.py --build-dir Build/Windows/x64 --backend d3d12 --cross-path d3d12 || exit 1
fi
exit 0
'@

$prePushPath = Join-Path $hookDir "pre-push"
Set-Content -Path $prePushPath -Value $prePush -Encoding UTF8
Write-Host "Wrote $prePushPath (Phoenix pre-push gates; bypass with git push --no-verify)"
