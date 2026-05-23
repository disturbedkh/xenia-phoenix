# Agent: gpu_edram

## Mission

GPU trace replay zero drift; legal retail `.xtr` captures; EDRAM / RTV-ROV correctness (Phase 1.2 bootstrap done).

## Read first

1. [re/20_xenos_gpu.md](../re/20_xenos_gpu.md)
2. [findings/universal/gpu_edram.md](../findings/universal/gpu_edram.md) — open `U-GPU-*` patterns
3. [dev/observability.md](../dev/observability.md) — invariants + `log summarize` for BF2 triage
4. [plan/bf2_gpu_roadmap.md](../plan/bf2_gpu_roadmap.md) — BF2 tracks, Edge parity, visual gate status
5. [xenia-phoenix-src/tests/gpu_traces/CORPUS.md](../../xenia-phoenix-src/tests/gpu_traces/CORPUS.md)
6. [plan/60_smoke_titles.md](../plan/60_smoke_titles.md) gpu_xtr column → [findings/games/](../findings/games/INDEX.md)

## Owns

- Synthetic corpus green in `tests/gpu_traces/golden/last_report.json`
- Retail trace entries (user-supplied, not committed by default)

## Commands

```powershell
cd "G:\Dev\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
phoenixctl repro capture --title-id <ID> --post-only --json
phoenixctl log summarize --title-id <ID> --write-summary
powershell -File tools/tier0/run_gpu_replay.ps1
python tools/gpu_replay_ci/validate_traces.py
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| Capture during gameplay | `gameplay_smoke` |
| RE / resolve semantics | `re_curator` |

## Stop and ask human

- No legal path to capture retail `.xtr`
- RTV/ROV policy change (Q3 in open questions)
