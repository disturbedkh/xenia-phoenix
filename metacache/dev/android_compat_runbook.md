# Android compat runbook

Canonical tree: `xenia-phoenix-src/`. Plan: [android_compat_roadmap.md](../plan/android_compat_roadmap.md).

## Prerequisites

- Android Studio (AGP 7.2+) or NDK r25 + CMake 3.22+
- Device or AVD with Vulkan 1.0+
- Submodules initialized: `git submodule update --init --recursive`

## Android Studio

1. Open `xenia-phoenix-src/android/android_studio_project/`
2. Sync Gradle (downloads NDK if needed)
3. Build variant: `debug` or `release`, ABI `arm64-v8a`
4. Run on device — launcher → **Open emulator** or **Open game**

Native build invokes root CMake with target `xenia-app` → `libxenia-app.so`.

## Command-line NDK (CI parity)

```bash
cd xenia-phoenix-src
export ANDROID_NDK_ROOT=/path/to/ndk/25.2.9519653
cmake -S . -B build-android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DCMAKE_BUILD_TYPE=Release \
  -DXENIA_BUILD_TESTS=OFF \
  -DXENIA_BUILD_MISC=OFF
cmake --build build-android-arm64 --target xenia-app -j$(nproc)
```

## Logcat

```bash
adb logcat -s xenia:* XeniaRuntimeException:* AndroidRuntime:E
```

## Launch cvars (Bundle extras)

Pass via `WindowedAppActivity.EXTRA_CVARS`:

- `target` — content URI or path to `.iso` / `.xex`
- `apu=android`, `gpu=vulkan`, `hid=android` (launcher sets these for game open)
- `target_trace_file` — GPU trace `.xtr` URI for trace viewer activity

## JIT validation checklist

See [android_status.md](../../xenia-phoenix-src/docs/android_status.md) § JIT / memory validation.
