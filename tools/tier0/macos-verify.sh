#!/usr/bin/env bash
# Post-build verification for macOS Apple Silicon builds.
set -euo pipefail

BUILD_DIR="${1:-build}"
BIN="${BUILD_DIR}/bin/macOS/Release"
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
