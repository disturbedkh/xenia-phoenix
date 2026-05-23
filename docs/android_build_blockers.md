# Android build blockers matrix

Tracked fixes for Phase 3 **Build 3.x** (green `libxenia-app.so` arm64 Release).

**Legend:** configure | compile | link | runtime

| ID | Category | Symptom | Fix | Status |
|----|----------|---------|-----|--------|
| A01 | configure | `<ranges>` not found (API 24, NDK r25) | NDK **r26c**, `ANDROID_PLATFORM=android-26`, `_LIBCPP_DISABLE_AVAILABILITY` | Fixed 2026-05-16 |
| A02 | configure | `glslangValidator` missing in CI | `apt install glslang-tools spirv-tools` | Fixed 2026-05-16 |
| A03 | configure | `version.h` missing | `tools/build/generate_version_h.py` at CMake configure | Fixed 2026-05-16 |
| A04 | compile | `PTHREAD_MUTEX_ROBUST` on Bionic | Guard robust mutex in `threading_posix.cc` | Fixed 2026-05-16 |
| A05 | compile | `std::jthread` unavailable | Use `std::thread` on Android in `xam_info.cc` | Fixed 2026-05-16 |
| A06 | compile | fontconfig / GTK in `imgui_drawer` | `#if XE_PLATFORM_LINUX && !XE_PLATFORM_ANDROID` | Fixed 2026-05-16 |
| A07 | compile | `vk_video/*.h` not found | Global include `Vulkan-Headers/include` | Fixed 2026-05-16 |
| A08 | link | Duplicate `SetAttributes` posix/android | `#if !XE_PLATFORM_ANDROID` in `filesystem_posix.cc` | Fixed 2026-05-16 |
| A09 | link | Duplicate `WindowedApp::creators_` | Drop duplicate `windowed_app_android.cc` from `xenia-app` | Fixed 2026-05-16 |
| A10 | link | ALSA factory on Android | `#if LINUX && !ANDROID` in `xenia_main.cc` | Fixed 2026-05-16 |
| A11 | compile | HID `X_INPUT_STATE` field names | Match `input.h` (`thumb_lx`, `packet_number`) | Fixed 2026-05-16 |
| A12 | compile | AAudio `Release` / `setVolume` API | Two-arg `Semaphore::Release`; `setVolume` API 29+ | Fixed 2026-05-16 |
| A13 | link | `-flto=thin` on Android NDK | Skip LTO when `ANDROID` in root `CMakeLists.txt` | Fixed 2026-05-16 |

## Repro (Docker, CI parity)

```bash
docker run --rm -v "$PWD:/src" -w /src ubuntu:24.04 bash /src/tools/docker/android-ndk-build.sh
```

Success: `build-android-arm64/bin/Android/libxenia-app.so`

## Policy

- **minSdk 26** (C++20 + AAudio). Vulkan-capable devices on Android 8.0+.
- **NDK r26c** in CI and Gradle `ndkVersion`.
