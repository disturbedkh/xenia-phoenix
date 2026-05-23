# Tier 1 — Gameplay gate (after Tier 1-PC)

Tier 1-PC closes every mission criterion that does not require playing a title. This document is the **single checklist** for the gameplay phase that finishes Tier 1 per `00_mission.md`.

## Prerequisites (Tier 1-PC must be green)

- [x] `docs/vmx128_fuzz_report.json` — 1M/op, zero divergences, `vmx128_fuzz_rm_pass=all` (local `run_vmx128_full_sweep.ps1`, Session 20). **PR CI:** 50k smoke via `tier0-differential` / `tier0-windows.yml`; 1M = release/manual or weekly workflow.
- [x] `xma2-diff` / `run_xma2_diff.ps1` — fixtures in `tests/xma2_packets/` (Tier 0 CI step).
- [x] `python tools/gpu_replay_ci/validate_traces.py` — synthetic corpus OK; committed `tests/gpu_traces/golden/last_report.json`.
- [x] `docs/patch_debt_dashboard_data.json` — full `game-patches` categorization committed.
- [x] `docs/kernel_stub_inventory.json` — static kStub export list.
- [x] Central `LogKernelStubHitGuest` on all `kStub` trampolines when `--kernel_stub_hit_log` is set.
- [x] CI: `tier0-differential` on PR (Orchestrator) + manual `tier0-windows.yml` — cpu-tests, vmx128 50k, xma2-diff, GPU D3D12 replay, stub fixture aggregate.

**Pre-gameplay closure (2026-05-16):** Tier 1-PC differential gates are green on x64 Windows. Enter §1–7 below.

## 1. Smoke roster

1. Fill [60_smoke_titles.md](60_smoke_titles.md) with titles you own (minimum 5 recommended).
2. Per title record: `title_id`, ISO path, GPU trace slot (optional), notes.
3. Generate patch triage list:

```powershell
python tools/tier0/list_smoke_patches.py --title-id 4D5307D1 5454082B ...
```

## 2. Per-title capture (≈5 minutes each)

From a Release build of `xenia.exe`:

```powershell
.\tools\tier0\run_smoke_capture.ps1 `
  -XeniaExe build\bin\Windows\Release\xenia_canary.exe `
  -GamePath "D:\games\TITLE\default.xex" `
  -TitleId "4D5307D1" `
  -DurationSec 300
```

Telemetry writes under `telemetry/` (gitignored). See [telemetry/README.md](../../xenia-phoenix-src/telemetry/README.md).

Collect:

| Artifact | Cvar / output | Pass criterion |
|----------|---------------|----------------|
| Kernel stubs | `--kernel_stub_hit_log=smoke_<id>.jsonl` | **Empty** file after session (or only known-baseline rows) |
| XMA divergences | `--apu_xma_divergence_log=smoke_<id>_xma.jsonl` | **Empty** |
| PCM stability | `--apu_pcm_hash_log=smoke_<id>_pcm.jsonl` | Stable 30s window vs prior baseline (`compare_pcm_hash_log.py`) |

Aggregate stubs:

```powershell
python tools/tier0/aggregate_stub_hits.py smoke_*.jsonl --out smoke_stub_summary.json
```

## 3. Patch debt retirement (categories C + D)

**Order (Q6):** smoke-linked **C** patches first, then cluster by `(module, export)` from stub aggregates. Full tree (988 C + 80 D) is stretch.

For each category **C** (emulator bug) and **D** (unknown) patch from `docs/patch_debt_dashboard_data.json` linked to your smoke titles:

1. Disable the patch in the patch DB / TOML.
2. Reproduce the original failure with telemetry (stub log, GPU trace, XMA log).
3. Fix root cause in Phoenix (CPU / GPU / kernel / APU).
4. Delete the patch entry; log transition in [30_debt_ledger.md](30_debt_ledger.md) and [cache/session_log.md](../cache/session_log.md).

**Target:** `counts_by_category.C + counts_by_category.D == 0` for patches affecting the smoke set; full tree zero is stretch.

## 4. Retail GPU traces

1. Capture legal retail `.xtr` per [CORPUS.md](../../xenia-phoenix-src/tests/gpu_traces/CORPUS.md) slots used by your smoke titles.
2. Run full replay gate:

```powershell
python tools/gpu_replay_ci/run.py --build-dir build --cross-path d3d12
```

3. Commit updated `tests/gpu_traces/golden/last_report.json` when RTV/ROV drift is zero.

## 5. New-title boot (mission criterion #5)

1. Pick a title **not** in `game-patches` and not in the smoke roster.
2. Boot to menu or first interactive frame without adding a new patch.
3. If it fails: capture stub/XMA logs, file a C-category patch only after opening a tracking issue (do not leave silent hacks).

## 6. Optional — devkit / homebrew goldens

When legal paired XMA/PCM blobs arrive from a 360 contact:

1. Add under `tests/xma2_packets/` with provenance in [30_xma2_audio.md](../re/30_xma2_audio.md).
2. Extend `gen_fixture_packets.py` and re-run `xma2-diff`.

## 7. Tier 1 “minimally achieved” sign-off

Update [20_tier_roadmap.md](20_tier_roadmap.md) **Tier 1-Gameplay** section and [cache/session_log.md](../cache/session_log.md) when all of:

- [ ] Smoke roster filled; per-title stub + XMA logs empty (or baselined)
- [ ] PCM hashes stable on smoke set
- [ ] Zero C+D patches for smoke titles (or documented exceptions)
- [ ] At least one retail GPU trace slot filled and replay-green
- [ ] One new-title boot without new patch

## Explicitly out of scope here

- Tier A HDMI capture-card ground truth
- Tier B modded-console traces
- Full `game-patches` tree retirement beyond smoke-linked titles
