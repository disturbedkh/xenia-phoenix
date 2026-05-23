#!/usr/bin/env bash
# Launcher for Xenia (Phoenix tree) on Linux — sets common environment defaults.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

export GDK_BACKEND="${GDK_BACKEND:-x11}"

if [[ -z "${VULKAN_SDK:-}" ]] && [[ -d "${HOME}/vulkan-sdk" ]]; then
  _ver="$(ls -1 "${HOME}/vulkan-sdk" 2>/dev/null | head -n1)"
  if [[ -n "${_ver}" && -d "${HOME}/vulkan-sdk/${_ver}/x86_64" ]]; then
    export VULKAN_SDK="${HOME}/vulkan-sdk/${_ver}/x86_64"
    export PATH="${VULKAN_SDK}/bin:${PATH}"
    export LD_LIBRARY_PATH="${VULKAN_SDK}/lib:${LD_LIBRARY_PATH:-}"
  fi
fi

CONFIG="${XENIA_CONFIG:-Release}"
BINARY="${ROOT}/build/bin/Linux/${CONFIG}/xenia_canary"

if [[ ! -x "${BINARY}" ]]; then
  echo "xenia_canary not found at ${BINARY}" >&2
  echo "Build with: ./xenia-build.py build --config=${CONFIG}" >&2
  exit 1
fi

exec "${BINARY}" "$@"
