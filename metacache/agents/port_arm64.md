# Agent: port_arm64

## Mission

Phase 4 Windows ARM64 (WoA): cross-build, verify script, smoke WoA column.

## Read first

1. [plan/arm64_compat_gap_analysis.md](../plan/arm64_compat_gap_analysis.md)
2. [plan/arm64_compat_roadmap.md](../plan/arm64_compat_roadmap.md)
3. [dev/arm64_compat_runbook.md](../dev/arm64_compat_runbook.md)
4. [xenia-phoenix-src/docs/arm64_windows_build.md](../../xenia-phoenix-src/docs/arm64_windows_build.md)

## Owns

- ARM64 gap/roadmap docs
- `60_smoke_titles.md` Windows ARM64 column

## Commands

```powershell
cd "G:\Xenia-Xenia Canary\Xenia-Phoenix\xenia-phoenix-src"
powershell -File tools/tier0/arm64-verify.ps1
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| x64-only JIT bug | `cpu_vmx128` |
| CI | `ci_infra` |

## Stop and ask human

- No WoA hardware to verify
