# Android compat gap analysis (Phase 3.0)

Date: 2026-05-16 (updated after Build 3.x green). Canonical tree: **`xenia-phoenix-src/`**.

## Build matrix (current vs target)

| Config | Gradle / NDK | CI | Phoenix source |
|--------|--------------|-----|----------------|
| Release arm64-v8a | CMake via AGP, NDK r26c, API 26 | [Android_arm64.yml](../../xenia-phoenix-src/.github/workflows/Android_arm64.yml) | **Green** (Docker 2026-05-16) |
| Release x86_64 | AVD | Optional | Same CMake |
| Debug | Android Studio | Not in CI | Same |

## C++ inventory (`*_android` under `src/xenia/`)

| Area | Status |
|------|--------|
| Platform / JNI / FS / UI / Vulkan | **Done** |
| `hid/android`, `apu/android` | **Done** (touch bridge + AAudio) |
| `StageAndroidLaunchPath` (SAF → cache) | **Done** |
| JIT on device | **Validate** (manual) |

## CMake / build

| Item | Status |
|------|--------|
| `ANDROID` branch, `xe_platform_sources` | **Done** |
| `xenia-app` SHARED + whole-archive `xenia-ui` | **Done** |
| NDK r26c, minSdk/API 26, no Android LTO | **Done** |
| `generate_version_h.py` at configure | **Done** |
| FFmpeg NEON (arm64) | **Done** in green build |

## Product / Java

| Item | Status |
|------|--------|
| `EmulatorActivity`, launcher, SAF | **Done** |
| Gradle `externalNativeBuild` | **Done** |

## Open / manual

- Device QA matrix: [android_device_qa.md](../../xenia-phoenix-src/docs/android_device_qa.md)
- APK in GitHub Release zip: deferred
- Physical gamepad via `InputDevice`: later

## Policy

Tier 1 stays **Windows-first**. Android does not block Tier 1 until device smoke is needed for a specific title.
