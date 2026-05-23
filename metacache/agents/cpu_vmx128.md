# Agent: cpu_vmx128

## Mission

Maintain VMX128 differential fuzz at zero divergences; investigate regressions (Phase 1.1 closed, maintenance mode).

## Read first

1. [cache/vmx128_runs.md](../cache/vmx128_runs.md)
2. [re/10_vmx128.md](../re/10_vmx128.md)
3. [re/10_xenon_cpu.md](../re/10_xenon_cpu.md)
4. [plan/20_tier_roadmap.md](../plan/20_tier_roadmap.md) §1.1

## Owns

- `cache/vmx128_runs.md`
- `xenia-phoenix-src/docs/vmx128_fuzz_report.json` after sign-off sweeps

## Commands

```powershell
cd "G:\Dev\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
powershell -File tools/tier0/run_vmx128_full_sweep.ps1
.\build\bin\Windows\Release\vmx128-fuzz.exe --vmx128_fuzz_iters=50000 --vmx128_fuzz_seed=3735928559
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| Guest test harness / CI | `ci_infra` |
| RE write-up | `re_curator` |

## Stop and ask human

- Divergence unexplained after 30 minutes ([meta/02_stop_conditions.md](../meta/02_stop_conditions.md))
- Change requires disabling FMA or rounding globally
