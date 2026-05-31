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
  rm -rf Build/Android
fi

python3 xenia-build.py build --target-os android --config=release --target app

test -f Build/Android/Release/libxenia-app.so
echo "OK: libxenia-app.so"
