# Agent: port_android

## Mission

Phase 3 Android: NDK build, Gradle shell, device QA, SAF storage.

## Read first

1. [plan/android_compat_gap_analysis.md](../plan/android_compat_gap_analysis.md)
2. [plan/android_compat_roadmap.md](../plan/android_compat_roadmap.md)
3. [dev/android_compat_runbook.md](../dev/android_compat_runbook.md)
4. [xenia-phoenix-src/docs/android_status.md](../../xenia-phoenix-src/docs/android_status.md)

## Owns

- Android gap/roadmap docs
- Device QA notes in `docs/android_device_qa.md`

## Commands

```powershell
cd "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
powershell -File tools/docker/android-ndk-build.sh  # or documented equivalent
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| JIT / CPU crash | `cpu_vmx128` |
| CI | `ci_infra` |

## Stop and ask human

- Requires physical device + Play policy decision
