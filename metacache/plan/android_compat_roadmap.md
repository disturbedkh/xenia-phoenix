# Android compatibility roadmap (Phoenix)

Numbering: **Phase 3** = Android host port (global phase in [20_tier_roadmap.md](20_tier_roadmap.md)). Not Linux Phase 2, not Tier 1 Phase 1.3.

Machine state: [xenia-phoenix-src/docs/android_status.md](../../xenia-phoenix-src/docs/android_status.md).

## Phase 3.0 — Gap analysis

- [x] [android_compat_gap_analysis.md](android_compat_gap_analysis.md)

## Phase 3.1 — AI brain structure

- [x] This roadmap
- [x] [20_tier_roadmap.md](20_tier_roadmap.md) Phase 3 table
- [x] [INDEX.md](../INDEX.md) layout

## Build 3.x — NDK + `libxenia-app.so`

- [x] `cmake/XeniaHelpers.cmake` Android `*_android` + `*_posix`
- [x] Root `CMakeLists.txt` ANDROID (log, dl, visibility, min API 24)
- [x] `xenia-app` SHARED on Android; link `xenia-cpu-backend-a64` / x64 per ABI
- [x] `android/android_studio_project` Gradle + CMake `externalNativeBuild`
- [x] First green arm64-v8a Release build (Docker / local NDK r26c, 2026-05-16)

## Product 3.x — Emulator shell

- [x] `EmulatorActivity` → windowed app `xenia`
- [x] Scoped storage roots (`files` / `cache` via JNI)
- [x] SAF file picker intent (`file_picker_android`)
- [x] Launcher: open game + demos (trace viewer, window demo)

## Runtime 3.x — Playable path

- [x] `hid/android` (touch → virtual gamepad)
- [x] `apu/android` (AAudio API 26+)
- [ ] JIT / exception handling validated on arm64 hardware
- [x] Document Vulkan device requirements in `android_status.md`

## QA 3.x

- [ ] GPU trace viewer on device
- [ ] Window Vulkan demo on device
- [ ] One smoke title boots (arm64 hardware)

## Packaging 3.x

- [x] `.github/workflows/Android_arm64.yml`
- [x] Orchestrator `build-android` → `Android_arm64.yml`; APK zip in Create_release deferred

## Policy

Tier 0/1 accuracy work stays **Windows-first** until **Build 3.x** Release is green. Android game QA does not block Tier 1 on Windows.
