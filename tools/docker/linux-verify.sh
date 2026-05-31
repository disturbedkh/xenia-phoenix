#!/bin/bash
# Post-build verification: runtime smoke, launcher, optional cpu-tests, AppImage.
set -euo pipefail

CONFIG="${1:-release}"
CONFIG_TITLE="$(echo "$CONFIG" | awk '{print toupper(substr($0,1,1)) substr($0,2)}')"
MODE="${2:-post-build}"

ensure_vulkan_sdk() {
  if [ -n "${VULKAN_SDK:-}" ] && [ -d "${VULKAN_SDK}" ]; then
    export PATH="${VULKAN_SDK}/bin:${PATH}"
    return
  fi
  if [ -d /root/vulkan-sdk ] && [ -n "$(ls -A /root/vulkan-sdk 2>/dev/null)" ]; then
    local ver
    ver=$(ls /root/vulkan-sdk | head -n1)
    export VULKAN_SDK="/root/vulkan-sdk/${ver}/x86_64"
    export PATH="${VULKAN_SDK}/bin:${PATH}"
    export LD_LIBRARY_PATH="${VULKAN_SDK}/lib:${LD_LIBRARY_PATH:-}"
  fi
}

install_runtime_deps() {
  apt-get update -qq
  apt-get install -y -qq \
    xvfb libsdl2-2.0-0 libgtk-3-0 libvulkan1 mesa-vulkan-drivers \
    libasound2t64 libfontconfig1 libfuse2 coreutils
  ensure_vulkan_sdk
}

run_with_timeout() {
  local seconds="$1"
  shift
  timeout --foreground "${seconds}" "$@"
}

# --help prints to stdout only when stdin is a TTY; otherwise SDL message boxes hang in CI.
run_help_smoke() {
  local seconds="$1"
  shift
  export GDK_BACKEND="${GDK_BACKEND:-x11}"
  if command -v script >/dev/null 2>&1; then
    local quoted=()
    for arg in "$@"; do quoted+=("$(printf '%q' "$arg")"); done
    run_with_timeout "${seconds}" xvfb-run -a script -qefc "${quoted[*]}" /dev/null
  else
    run_with_timeout "${seconds}" xvfb-run -a "$@"
  fi
}

smoke_binary() {
  local binary="$1"
  if [ ! -f "$binary" ]; then
    echo "Smoke FAILED: missing $binary" >&2
    return 1
  fi
  chmod +x "$binary"
  echo "Smoke: $binary --help"
  run_help_smoke 120 "$binary" --help
}

smoke_launcher() {
  if [ ! -f scripts/xenia-phoenix-linux.sh ]; then
    return 0
  fi
  sed -i 's/\r$//' scripts/xenia-phoenix-linux.sh 2>/dev/null || true
  chmod +x scripts/xenia-phoenix-linux.sh
  export XENIA_CONFIG="${CONFIG_TITLE}"
  echo "Smoke: scripts/xenia-phoenix-linux.sh --help"
  run_help_smoke 120 env XENIA_CONFIG="${CONFIG_TITLE}" bash scripts/xenia-phoenix-linux.sh --help
}

smoke_appimage() {
  local appimage="artifacts/release/xenia_canary_linux.AppImage"
  if [ ! -f "$appimage" ]; then
    echo "AppImage smoke: skipped (no $appimage)"
    return 0
  fi
  chmod +x "$appimage"
  echo "Smoke: $appimage --help"
  run_help_smoke 120 "$appimage" --help
}

run_cpu_tests() {
  echo "Building xenia-cpu-tests (${CONFIG})..."
  python3 xenia-build.py build --config="${CONFIG}" --build-tests \
    --target xenia-cpu-tests
  echo "Running xenia-cpu-tests..."
  python3 xenia-build.py test --no_build --config="${CONFIG}" --target xenia-cpu-tests
}

run_post_build_verify() {
  cd /src
  install_runtime_deps

  local binary="$(python3 /src/tools/build/xenia_paths.py bin --config "${CONFIG_TITLE}" --os Linux)/xenia_canary"
  smoke_binary "$binary"
  smoke_launcher

  if [ "${CONFIG,,}" = "release" ]; then
    run_cpu_tests
    smoke_appimage
  fi

  echo "Verify OK (${CONFIG})"
}

case "$MODE" in
  post-build)
    run_post_build_verify
    ;;
  smoke-only)
    cd /src
    install_runtime_deps
    smoke_binary "$(python3 /src/tools/build/xenia_paths.py bin --config "${CONFIG_TITLE}" --os Linux)/xenia_canary"
    smoke_launcher
    ;;
  cpu-tests-only)
    cd /src
    export CC="${CC:-clang-20}"
    export CXX="${CXX:-clang++-20}"
    sed -i 's/\r$//' xenia-build.py 2>/dev/null || true
    ensure_vulkan_sdk
    run_cpu_tests
    echo "CPU tests OK (${CONFIG})"
    ;;
  *)
    echo "Usage: $0 <release|debug|checked> [post-build|smoke-only|cpu-tests-only]" >&2
    exit 1
    ;;
esac
