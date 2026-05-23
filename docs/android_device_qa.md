# Android device QA matrix (Phase 3 QA 3.x)

Run on **physical arm64** hardware after a green [android_build_blockers.md](android_build_blockers.md) build. Install debug APK from Android Studio (`android/android_studio_project`).

| Step | Action | Pass |
|------|--------|------|
| C1 | Launcher → **Window Vulkan demo** | Renders, no native crash |
| C2 | **GPU trace viewer** + `target_trace_file` content URI | Trace loads |
| C3 | **Open emulator** (no game) | UI / idle |
| C4 | **Open game** (owned `.xex` / `.iso` via SAF) | Boots past loader |
| C5 | JIT checklist ([android_compat_runbook.md](../metacache/dev/android_compat_runbook.md)) | No immediate SIGSEGV |

Logcat: `adb logcat -s xenia:* XeniaRuntimeException:* AndroidRuntime:E`

Record smoke title in [60_smoke_titles.md](../metacache/plan/60_smoke_titles.md) Android column when C4 passes.
