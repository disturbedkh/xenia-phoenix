#!/usr/bin/env bash
# Resolve Phoenix bin directory via tools/build/xenia_paths.py
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONFIG="${1:-Release}"
TARGET_ARCH="${2:-}"
ARGS=(bin --config "$CONFIG")
if [[ -n "$TARGET_ARCH" ]]; then
  ARGS+=(--target-arch "$TARGET_ARCH")
fi
python3 "$ROOT/tools/build/xenia_paths.py" "${ARGS[@]}"
