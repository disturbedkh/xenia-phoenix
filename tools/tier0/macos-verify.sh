#!/usr/bin/env bash
# Post-build verification for macOS Apple Silicon builds.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$(python3 "${ROOT}/tools/build/xenia_paths.py" bin --config Release --os macOS --target-arch arm64)"
EXE="${BIN}/xenia_canary"
CPU_TESTS="${BIN}/xenia-cpu-tests"

if [[ ! -f "$EXE" ]]; then
  echo "Missing $EXE" >&2
  exit 1
fi

chmod +x "$EXE"
echo "Smoke: $EXE --help"
"$EXE" --help >/dev/null

if [[ -f "$CPU_TESTS" ]]; then
  echo "CPU tests: $CPU_TESTS"
  chmod +x "$CPU_TESTS"
  (cd "$BIN" && ./xenia-cpu-tests)
else
  echo "Warning: $CPU_TESTS not found"
fi

echo "macos-verify: OK"
