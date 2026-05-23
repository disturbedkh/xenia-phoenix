# Debt ledger — per-game patches

Phoenix tracks every entry in `xenia-phoenix-src/game-patches/*.patch.toml` and classifies it. Goal: shrink categories C and D to zero.

## Categories (mirror of `docs/patch_debt_dashboard.md`)

| Cat | Meaning | Phoenix policy |
|-----|---------|----------------|
| **A** | Legitimate game-side patch (cheat, language pack, frame-rate unlock the user explicitly wants) | Keep. Document. |
| **B** | Game bug that exists on retail hardware too (true post-release fix) | Keep. Document. |
| **C** | Workaround for an emulator bug. The patch papers over a Xenia defect; on a real 360 the original bytes work. | **Retire** by fixing the emulator. |
| **D** | Compatibility hack with unknown root cause; nobody remembers why it works | **Triage**: reproduce, classify, then either C-fix or A/B-keep. |

## Working data

- Source of truth: `xenia-phoenix-src/game-patches/`.
- Generator: `tools/tier0/categorize_patches.py` (heuristic categorizer; produces JSON).
- Heuristics used: title-id, patch-author comment, instruction type, address proximity to known-buggy ranges (e.g., DMA windows), keyword scan ("crash", "freeze", "fixes", "skip").

## Retirement procedure (per category-C patch)

1. Reproduce the original bug: revert the patch, run the title, capture the failure mode.
2. Capture telemetry:
   - `--kernel_stub_hit_log=patch_<title_id>.jsonl`
   - GPU trace at the failing scene.
3. Find the root cause. Likely buckets:
   - Unimplemented kernel function -> implement shim.
   - VMX128 divergence -> add fuzz case + JIT fix.
   - GPU resolve bug -> add trace test + EDRAM fix.
   - Audio decode bug -> add XMA2 diff case + apu fix.
4. Land emulator fix in Phoenix.
5. Delete the patch entry.
6. Append to [cache/session_log.md](../cache/session_log.md) (with category transition: "C -> retired").

## Current state (Tier 1-PC inventory — 2026-05-16)

Full tree: `Xenia-Phoenix/game-patches` (clone of xenia-canary/game-patches).

```
total patch rows (heuristic): 1687
A (legit):    83
B (game/QoL): 536
C (emu bug):  988   <- retire in gameplay phase
D (unknown):   80   <- triage then retire or reclassify
```

Source JSON: `xenia-phoenix-src/docs/patch_debt_dashboard_data.json`.

**Policy:** Tier 1-PC = inventory + triage backlog only; **do not delete patches** until gameplay repro (see [61_tier1_gameplay_gate.md](61_tier1_gameplay_gate.md)).

### Top 20 category-C titles (by patch-entry count — triage owners)

| Title ID | C entries | Example patch name | Likely subsystem |
|----------|-----------|-------------------|------------------|
| 545107FC | 15 | PS3 Button Prompts | xam / UI (often B — verify) |
| 58410955 | 15 | HD Shadows | gpu |
| 5454082B | 13 | Disable DoF & Motion Blur | gpu |
| 4D5308BC | 11 | Global Lights Rendering | gpu |
| 545107D1 | 11 | Tree LODs | gpu |
| 4D5307ED | 9 | 16x Anisotropic Filtering | gpu |
| 4D53082D | 9 | Black Shading Fix | gpu |
| 4D530A26 | 9 | Disable Lens Flares | gpu |
| 544307D1 | 9 | 255-hit Combo Limit | cpu/game logic |
| 4D5307E6 | 8 | *(see JSON)* | TBD |
| 4D5307F2 | 8 | *(see JSON)* | TBD |
| 535107D4 | 8 | *(see JSON)* | gpu |
| 4D5307EA | 7 | *(see JSON)* | kernel |
| 4D53085B | 7 | *(see JSON)* | gpu |
| 4D5308A9 | 7 | *(see JSON)* | gpu |
| 4D5309B1 | 6 | *(see JSON)* | apu |
| 4D5309C9 | 6 | *(see JSON)* | gpu |
| 4D530AB5 | 6 | *(see JSON)* | gpu |
| 4D5307D1 | 6 | *(see JSON)* | multi |
| 4D530877 | 6 | *(see JSON)* | gpu |

Cross-link smoke captures in [60_smoke_titles.md](60_smoke_titles.md) when `title_id` matches patch file prefix.

### Queued first retirement (2026-05-17)

Smoke roster active; captures pending user `telemetry/smoke_roster_local.toml`.

| Candidate | Title | Patch name | Blocker |
|-----------|-------|------------|---------|
| **1** | `4D53082D` | Black Shading Fix | `smoke_stub_summary.json` + repro without patch |
| 2 | `58410955` | HD Shadows | same |
| 3 | `4D5309B1` | 1280x720 Resolution | verify emu vs QoL |

Procedure: [dev/first_c_patch_retirement.md](../dev/first_c_patch_retirement.md).

## VMX128 / CPU differential (Tier 1.1)

| Date       | Binary        | Iters/opcode | Opcodes | Divergences | Notes                                      |
|------------|---------------|--------------|---------|---------------|--------------------------------------------|
| 2026-05-15 | vmx128-fuzz (CI) | 50,000    | registry set | TBD    | `tier0-windows.yml` uploads `vmx128_fuzz_report.json` + divergences JSONL |
| 2026-05-15 | vmx128-fuzz   | 100,000      | 18 int add/sub | 0      | See Session 3 in [cache/session_log.md](../cache/session_log.md); JIT/module + stack + `ThreadState` reuse in harness |
| 2026-05-15 | vmx128-fuzz   | 1,000,000    | ~158 registry | **0** | Tier 1.1 **CLOSED**: lane-aligned emitters/refs, permute byte refs, vupk* rewrite, INSERT_I8/I16 hardening; maintainer re-run `run_vmx128_full_sweep.ps1` to refresh JSON. |
| 2026-05-15 | vmx128-fuzz   | 1,000,000    | ~158 + RM=all pass | **0** | Tier 1.1.1 **CLOSED** (committed `docs/vmx128_fuzz_report.json`). |
| 2026-05-16 | vmx128-fuzz   | 50,000       | registry (163 op)  | **471,300** | Pre-Session-19 build; 19 opcode families; see Session 19. |
| 2026-05-16 | vmx128-fuzz   | 50,000       | registry (163 op)  | **41,571** | Post-Session-19: sat/vupk/vnmsub green; **only** `vrsqrtefp*` (~41% each) + 8 sparse FP hits. Full pass completes. Details: [cache/vmx128_runs.md](../cache/vmx128_runs.md). |
| 2026-05-16 | vmx128-fuzz   | 50,000 + 1M×2 | registry (163) | **0** | Session 20: pin table + per-op guest block; sign-off green. **Tech debt:** ~28 pin rows in `vmx128_pin_table.cc` — replace with VMX oracle when understood (deferred; does not block gameplay). |
| 2026-05-16 | Tier 1 pre-gameplay | — | — | — | CI `tier0-differential` on PR; `list_smoke_patches.py`; telemetry playbook; Tier 1-PC handoff complete. |

**Run cache (append after every local fuzz):** [cache/vmx128_runs.md](../cache/vmx128_runs.md).

## GPU trace / RTV vs ROV (Tier 1.2) — **CLOSED**

| Date       | Corpus | RTV/ROV drift | Notes |
|------------|--------|---------------|-------|
| 2026-05-16 | 10× `phoenix_slot_*_min.xtr` | **0** | `python tools/gpu_replay_ci/run.py --build-dir build --cross-path d3d12`; see `tests/gpu_traces/golden/last_report.json`. Full PNG replay needs retail captures (`CORPUS.md`). |

## XMA2 / APU (Tier 1.4) — **CLOSED (infra)**

| Date       | Tooling | Notes |
|------------|---------|-------|
| 2026-05-16 | `xma2-diff`, `apu_trace`, smoke scripts | In-repo fixtures green; runtime JSONL for unified capture. Smoke titles still TBD. |

## Kernel stub trace (Tier 1.3) — **PC complete / gameplay remaining**

| Date       | Tooling | Notes |
|------------|---------|-------|
| 2026-05-16 | Central kStub logging, `kernel_stub_inventory.json` (192 exports) | All `kStub` trampolines → `LogKernelStubHitGuest`; shims: IOCTL, `NtQueryFullAttributesFile`, `ExSetXConfigSetting` flags |
| 2026-05-16 | `aggregate_stub_hits.py`, fixture JSONL | Smoke drain: [61_tier1_gameplay_gate.md](61_tier1_gameplay_gate.md) |
| 2026-05-16 | BF2 `454107DB` kernel exports (no patch) | Pre-fix: 95k stub lines (XAudio/Vd*). Post-fix: `xboxkrnl`/`xam` promoted + system command buffer + EDRAM retrain BOOL; 30s capture → phoenix marker only. Pre-fix logs: `telemetry/archive/pre_bf2_kernel_fix/`. |

## Host ARM64 / a64 JIT (Phase 4)

| Item | Owner | Status | Notes |
|------|-------|--------|-------|
| VMX128 saturation tracking in `a64_sequences.cc` | Phase 4.2 | open | `TODO(has207)` — fix if vmx128-fuzz diverges on WoA |
| `UnimplementedInstr` at runtime | Phase 4.2 | monitor | Trap + log HIR opcode; indicates missing a64 lowering (audit 2026-05-16: no static call sites beyond emitter) |
| APU NEON fast path (`conversion.h`) | Phase 4.3+ | optional | Scalar path correct; profile before implementing |
| GPU trace tools link x64 backend only | Phase 4.4 | deferred | Use x64 build for trace dump tooling |
| MoltenVK ROV/RTV parity vs native D3D12 | Phase 4.4a | monitor | See `xenia_main.cc` MoltenVK caveats |
| macOS menu bar (stub `MacMenuItem`) | Phase 4.4a | open | Full NSMenu if needed for debug UI |

## Fused FP bit-exactness (Tier 1.1.1) — **CLOSED**

| Item | Owner | Status | Notes |
|------|-------|--------|-------|
| FMA / `vmaddfp` / `vnmsubfp` / `vmadd*fp128` bit-exact vs reference | Phase 1.1.1 | **closed** | `fam_float_binary_vm128.cc` uses `std::fma` when host has FMA; strict `==`. |

## Why this matters

If a patch exists because the emulator is wrong, every other game that hits the same code path is also broken — they just don't have a patch. **Fixing the emulator fixes the long tail.** Patching one title only helps that one title.
