# Testing and CI

Phoenix validates accuracy with differential tests and CI workflows — not ad-hoc “does it boot” checks alone.

Authoritative commands: [../metacache/dev/20_test_commands.md](../metacache/dev/20_test_commands.md)

## VMX128 fuzz

From `xenia-phoenix-src/`:

```powershell
$seed = 3735928559
$fuzz = .\build\bin\Windows\Release\vmx128-fuzz.exe

# 50k full registry smoke
& $fuzz --vmx128_fuzz_iters=50000 --vmx128_fuzz_seed=$seed `
  --vmx128_fuzz_report_out=docs/vmx128_fuzz_report_local.json

# 1M sign-off (both passes)
powershell -File tools/tier0/run_vmx128_full_sweep.ps1
```

Report artifact: `docs/vmx128_fuzz_report.json`

## Guest PPC tests

```powershell
.\build\bin\Windows\Release\xenia-cpu-tests.exe
```

## GPU trace replay

```powershell
python tools/gpu_replay_ci/validate_traces.py
python tools/gpu_replay_ci/run.py --build-dir build --format-validate-only
# Full RTV/ROV gate:
powershell -File tools/tier0/run_gpu_replay.ps1
```

Corpus: `tests/gpu_traces/` · Golden report: `tests/gpu_traces/golden/last_report.json`

## XMA2 diff

```powershell
powershell -File tools/tier0/run_xma2_diff.ps1
```

Fixtures: `tests/xma2_packets/`

## Kernel stub aggregate

```powershell
python tools/tier0/aggregate_stub_hits.py tests/kernel_stub/stub_hits_fixture.jsonl
```

Runtime capture uses `--kernel_stub_hit_log=` on `xenia_canary.exe` during smoke sessions.

## Patch triage (gameplay)

```powershell
python tools/tier0/list_smoke_patches.py --title-id <TITLE_ID> ...
```

## CI workflows

| Workflow | When |
|----------|------|
| `tier0-differential.yml` | Every PR — cpu-tests, vmx128 50k, xma2-diff, GPU replay |
| `tier0-windows.yml` | Manual — full parallel build + gates |
| `vmx128-1m-weekly.yml` | Weekly / manual — 1M opcode sweep |
| `tier0-checked.yml` | Manual — Checked `xenia-cpu-tests` |

Port workflows also exist under `.github/workflows/` (Linux, Android, ARM64, macOS).

## Tier 0 overview

See [../xenia-phoenix-src/docs/TIER0_README.md](../xenia-phoenix-src/docs/TIER0_README.md) for MVP map (VMX128, GPU replay, patch dashboard, stub JSONL).

## Deeper detail (agents)

- [../metacache/dev/20_test_commands.md](../metacache/dev/20_test_commands.md)
- [../metacache/dev/30_repo_layout.md](../metacache/dev/30_repo_layout.md) (CI section)
- Gameplay gate: [../metacache/plan/61_tier1_gameplay_gate.md](../metacache/plan/61_tier1_gameplay_gate.md)
