# Phoenix metrics

Every PR should move at least one metric. If none apply, explain in `plan/50_open_questions.md` before merge.

| Metric | Tool / artifact | Good direction |
|--------|-----------------|----------------|
| VMX128 coverage | `vmx128-fuzz` opcode count, samples/op | Up coverage; **zero** divergences |
| GPU replay | `tools/gpu_replay_ci/run.py`, `last_report.json` | Zero hash drift on corpus |
| Kernel stubs | `--kernel_stub_hit_log` on smoke set | Row count **down** (empty = goal) |
| Patch debt | `tools/tier0/categorize_patches.py` | Category **C+D count down** |
| XMA2 fixtures | `xma2-diff` | Zero divergences on fixtures |
| Smoke roster | `plan/60_smoke_titles.md` | More filled columns |

## CI gates

- PR path: `tier0-differential` (Orchestrator)
- Manual full: `tier0-windows.yml`
- Weekly 1M: `vmx128-1m-weekly.yml`

Local green ≠ merged. See [meta/00_agent_protocol.md](00_agent_protocol.md) § CI.
