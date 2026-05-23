#!/bin/bash
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive
LLVM_VERSION="${LLVM_VERSION:-20}"
UBUNTU_BASE="${UBUNTU_BASE:-noble}"
CONFIG="${1:-release}"
VERIFY_MODE="${2:-}"
CONFIG_TITLE="$(echo "$CONFIG" | awk '{print toupper(substr($0,1,1)) substr($0,2)}')"
SKIP_VERIFY="${SKIP_VERIFY:-0}"

setup_build_env() {
  apt-get update -qq
  apt-get install -y -qq wget gnupg software-properties-common ca-certificates git

  wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
  add-apt-repository -y "deb http://apt.llvm.org/${UBUNTU_BASE}/ llvm-toolchain-${UBUNTU_BASE}-${LLVM_VERSION} main"
  apt-get update -qq
  apt-get install -y -qq \
    build-essential mesa-vulkan-drivers valgrind libc++-dev libc++abi-dev \
    libgtk-3-dev libsdl2-dev libvulkan-dev libx11-xcb-dev liblz4-dev \
    libasound2-dev libfontconfig1-dev \
    clang-${LLVM_VERSION} lld-${LLVM_VERSION} llvm-${LLVM_VERSION} ninja-build cmake \
    spirv-tools python3 pkg-config

  update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 200
  update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 200
  update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VERSION} 200
  update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-${LLVM_VERSION} 200
  update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-${LLVM_VERSION} 200
  update-alternatives --install /usr/bin/llvm-ranlib llvm-ranlib /usr/bin/llvm-ranlib-${LLVM_VERSION} 200
  update-alternatives --install /usr/bin/llvm-nm llvm-nm /usr/bin/llvm-nm-${LLVM_VERSION} 200

  if [ -z "${VULKAN_SDK:-}" ] || [ ! -d "${VULKAN_SDK}" ]; then
    mkdir -p /root/vulkan-sdk
    if [ ! "$(ls -A /root/vulkan-sdk 2>/dev/null)" ]; then
      wget -qO /tmp/vulkan-sdk.tar.xz https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.xz
      tar -xf /tmp/vulkan-sdk.tar.xz -C /root/vulkan-sdk
    fi
    VULKAN_SDK_VERSION=$(ls /root/vulkan-sdk)
    export VULKAN_SDK="/root/vulkan-sdk/${VULKAN_SDK_VERSION}/x86_64"
  fi
  export PATH="${VULKAN_SDK}/bin:${PATH}"
}

if [ -z "${PHOENIX_LINUX_BUILD_ENV:-}" ]; then
  setup_build_env
fi

cd /src
git config --global --add safe.directory /src
sed -i 's/\r$//' xenia-build.py tools/docker/linux-verify.sh 2>/dev/null || true

if [ -f build/CMakeCache.txt ]; then
  if grep -qiE '(CMAKE_HOME_DIRECTORY:INTERNAL=)?[gG]:/' build/CMakeCache.txt 2>/dev/null; then
    echo "Removing build/ (Windows CMake cache)..."
    rm -rf build
  fi
fi

export CC=clang-${LLVM_VERSION}
export CXX=clang++-${LLVM_VERSION}

if [ -n "$VERIFY_MODE" ]; then
  case "$VERIFY_MODE" in
    verify-only) VERIFY_MODE=post-build ;;
    smoke-only|cpu-tests-only|post-build) ;;
    *)
      echo "Unknown verify mode: $VERIFY_MODE" >&2
      exit 1
      ;;
  esac
  bash tools/docker/linux-verify.sh "${CONFIG}" "${VERIFY_MODE}" 2>&1 | tee -a "/src/linux_verify_${CONFIG,,}.log"
  exit $?
fi

EXCLUDE="DirectXShaderCompiler"
SUBMODULES=$(grep 'path = ' .gitmodules | sed 's/^[[:space:]]*path = //' | tr -d '\r' | grep -vE "$EXCLUDE")
git submodule sync $SUBMODULES
git submodule update --init --depth=1 $SUBMODULES

LOG="/src/linux_build_${CONFIG,,}.log"
: >"$LOG"
set -o pipefail
python3 xenia-build.py doctor 2>&1 | tee -a "$LOG"
python3 xenia-build.py build --config="${CONFIG}" 2>&1 | tee -a "$LOG"
build_status=${PIPESTATUS[0]}

binary="/src/build/bin/Linux/${CONFIG_TITLE}/xenia_canary"
if [ "$build_status" -ne 0 ]; then
  echo "Build FAILED (status=$build_status)" >&2
  exit "${build_status:-1}"
fi
if [ ! -f "$binary" ]; then
  echo "Build FAILED: missing $binary" >&2
  exit 1
fi

echo "Build OK: $binary ($(stat -c%s "$binary") bytes)" | tee -a "$LOG"

if [ "$SKIP_VERIFY" != "1" ]; then
  sed -i 's/\r$//' tools/docker/linux-verify.sh 2>/dev/null || true
  bash tools/docker/linux-verify.sh "${CONFIG}" post-build 2>&1 | tee -a "$LOG"
fi
