# PR runbook

How the agent ships a change.

## Steps (in order)

1. **Branch.** `git checkout -b phoenix/<phase>/<short-name>` (e.g. `phoenix/1.1-cpu/vaddshs-saturation`).
2. **Implement** smallest possible change (single subsystem).
3. **Build + test** per `dev/10_build_commands.md` and `dev/20_test_commands.md`.
4. **Verify CI green locally (Tier 1-PC gate):**
   - `xenia-cpu-tests.exe`
   - vmx128-fuzz smoke (50k): `build/bin/Windows/vmx128-fuzz.exe --vmx128_fuzz_iters=50000`
   - **Release sign-off (before Tier 1-PC merge):** `tools/tier0/run_vmx128_full_sweep.ps1` (1M + RM=all) → commit `docs/vmx128_fuzz_report.{json,md}`
   - `tools/tier0/run_xma2_diff.ps1`
   - `python tools/gpu_replay_ci/validate_traces.py` and `python tools/gpu_replay_ci/run.py --build-dir build --format-validate-only` (full RTV/ROV: `run_gpu_replay.ps1` when trace-dump is built)
   - `python tools/tier0/categorize_patches.py` with `GAME_PATCHES_ROOT` → refresh `docs/patch_debt_dashboard_data.json` when patch triage changes
   - `python tools/tier0/aggregate_stub_hits.py tests/kernel_stub/stub_hits_fixture.jsonl`
5. **Write the PR description** using `docs/TIER0_PR_TEMPLATE.md` (it's the source of truth).
6. **Tag** with the phase label: `[1.1-cpu]`, `[1.2-gpu]`, `[1.3-kernel]`, `[1.4-audio]`, `[debt]`, `[infra]`.
7. **Push + open PR** in the Phoenix fork.
8. **Wait for `.github/workflows/tier0-windows.yml`** to pass.
9. **Update metacache** when merged: [cache/session_log.md](../cache/session_log.md) (1-3 lines), and any `re/*.md` you learned from.

## Template recap (lives in `docs/TIER0_PR_TEMPLATE.md`)

```
## Summary
<one-paragraph what + why>

## Root cause
<what bug / accuracy gap this fixes; cite source files and traces>

## Reproducer
<exact commands to see the failure on `main` and the fix on this branch>

## Performance
<host-fps / decode-rate / N-percent improvement or regression>

## Risk
<which titles are likely affected; what to test if you have a 360 + game lib>

## Legal
<confirms no leaked-SDK content, no piracy enablement, no signing material>
```

## Do not

- Open a PR with a `// HACK` that lacks a tracking issue.
- Disable CI steps to make a PR green.
- Update `c:\Users\khutt\.cursor\plans\xenia_tier_0_execution_plan_94722ff4.plan.md` (read-only by user instruction).
- Combine subsystems in one PR (CPU + GPU + kernel = three PRs).

## When to ask the human

- The change touches >100 files.
- The change deletes a kernel shim with callers.
- The change modifies the patcher schema (`src/xenia/patcher/patch_db.h`).
- A fuzz divergence cannot be explained after 30 minutes.

(Mirrors `plan/40_meta_goals.md` stop-conditions.)

## Phoenix branch naming convention

```
phoenix/<phase>/<area>-<gh-username-or-issue>
```

Examples:

- `phoenix/1.1-cpu/vmx128-vaddshs`
- `phoenix/1.2-gpu/edram-msaa-resolve`
- `phoenix/1.3-kernel/mmallocatephysicalmemoryex`
- `phoenix/debt/retire-tlou-jit-patch`
- `phoenix/infra/tier0-aarch64-cross-build` — **superseded (2026-05-16)** by merged `build-win_arm64.yml` + Orchestrator `build-windows-arm64`; use for follow-up CI fixes only
