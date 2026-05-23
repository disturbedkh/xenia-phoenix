#!/usr/bin/env bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq cmake ninja-build git python3 curl unzip ca-certificates spirv-tools glslang-tools

if [[ ! -d /opt/android-ndk-r26c ]]; then
  curl -fsSL -o /tmp/ndk.zip \
    https://dl.google.com/android/repository/android-ndk-r26c-linux.zip
  unzip -q /tmp/ndk.zip -d /opt
  rm /tmp/ndk.zip
fi
export ANDROID_NDK_ROOT=/opt/android-ndk-r26c

cd /src
git submodule update --init --recursive
if [[ "${ANDROID_BUILD_CLEAN:-0}" == "1" ]]; then
  rm -rf build-android-arm64
fi

cmake -S . -B build-android-arm64 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DCMAKE_BUILD_TYPE=Release \
  -DXENIA_BUILD_TESTS=OFF \
  -DXENIA_BUILD_MISC=OFF

cmake --build build-android-arm64 --target xenia-app -j"$(nproc)"

test -f build-android-arm64/bin/Android/libxenia-app.so
echo "OK: libxenia-app.so"
