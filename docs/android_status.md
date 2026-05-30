# Android build status (Phoenix)

Last updated: 2026-05-16.

## Requirements

- **Android 8.0+ (API 26)** — C++20 `<ranges>`, AAudio, NDK r26c
- **Vulkan 1.0** with `independentBlend`
- **arm64-v8a** primary; x86_64 for AVD testing

## Build (verified)

- Tree: `xenia-phoenix-src/`
- CI: [.github/workflows/Android_arm64.yml](../.github/workflows/Android_arm64.yml) (NDK r26c, `android-26`)
- Docker parity: `tools/docker/android-ndk-build.sh`
- Output: `Build/Android/Release/libxenia-app.so`
- Studio: `android/android_studio_project/` (`minSdk 26`, `ndkVersion 26.1.10909125`)

Blockers log: [android_build_blockers.md](android_build_blockers.md)

## Runtime backends (launcher Bundle defaults)

| Subsystem | Cvar | Backend |
|-----------|------|---------|
| GPU | `gpu=vulkan` | Vulkan only |
| APU | `apu=android` | AAudio |
| HID | `hid=android` | Touch → virtual gamepad |

## Playable path (code)

- `StageAndroidLaunchPath()` copies SAF `content://` into cache before launch
- Touch forwarded: `EmulatorWindow` → `InputSystem::OnAndroidTouch` → `AndroidInputDriver`

## JIT / device validation (manual)

See [android_device_qa.md](android_device_qa.md) and runbook JIT checklist.

- [ ] JIT / guest execution on physical arm64
- [ ] Optional: `memfd_create` on API 30+ ([memory_posix.cc](../src/xenia/base/memory_posix.cc) TODO)

## APK

Build/install via Android Studio (`assembleDebug` / `assembleRelease`). CI uploads `libxenia-app.so` only; APK packaging in release workflow is deferred.
