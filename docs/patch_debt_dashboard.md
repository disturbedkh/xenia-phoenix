# Patch compatibility debt dashboard (Tier 0 / MVP3)

This document is a **methodology + template** for classifying patches from
[`xenia-canary/game-patches`](https://github.com/xenia-canary/game-patches) and
linking them to upstream emulator work.

## Categories

| Code | Meaning |
|------|---------|
| **A** | Anti-tamper / DRM / retail-only checks (legitimate patch; not an emulator bug). |
| **B** | Quality of life (skip intro, FPS cap, widescreen, etc.). |
| **C** | **Emulator-bug compensation** — retail game is correct; patch masks missing/wrong Xenia behavior (Phoenix policy: **retire** by fixing the emulator). |
| **D** | Unknown root cause — triage to C, A, or B before keeping. |

## Automated first pass

Run (after cloning `game-patches` next to this repo or set `GAME_PATCHES_ROOT`):

```powershell
$env:GAME_PATCHES_ROOT = "D:\src\game-patches"
python tools/tier0/categorize_patches.py --out docs/patch_debt_dashboard_data.json

Smoke gameplay triage (category C+D for owned titles):

```powershell
python tools/tier0/list_smoke_patches.py --title-id 4D5307D1 5454082B
```
```

The script uses heuristics only (description keywords, NOP/branch patterns in
patch data). **All category D rows need human review** before filing issues.

## Top root-cause themes (2026-05-30 � `game-patches` clone)

Counts: **C=598**, **D=61**, **B=945**, **A=83** (`docs/patch_debt_dashboard_data.json`). (2026-05-30 full `game-patches` tree.)

1. **GPU / post-processing** — lens flare, motion blur, DoF, shadow/LOD hacks (often category C mis-tagged as visual QoL; verify on hardware).
2. **Input / UI cosmetics** — PS3 button prompts, widescreen patches (many are B, not emu debt).
3. **Kernel / file / IOCTL** — mount, cache, `NtDeviceIoControlFile` paths; tie to `kernel_stub_inventory.json` + smoke stub logs.
4. **Audio / XMA** — decode edge cases; use `xma2-diff` fixtures + `--apu_xma_divergence_log` on repro.
5. **CPU / VMX128** — rare; confirm with `vmx128-fuzz` before blaming JIT.

## Next actions

- For each **D** patch: open or link a `xenia-canary` issue describing the
  root-cause subsystem, and add the issue URL to the patch TOML comment block
  (convention TBD with maintainers).
- Prefer fixing root cause over growing category D.

## Latest automated run (Tier 0 closeout)

Generated from `tools/tier0/fixtures/patch_debt/` (smoke fixtures; replace with
`GAME_PATCHES_ROOT` pointing at [`xenia-canary/game-patches`](https://github.com/xenia-canary/game-patches) for real counts):

```powershell
python tools/tier0/categorize_patches.py --patches-root tools/tier0/fixtures/patch_debt --out docs/patch_debt_dashboard.json
```

See [`patch_debt_dashboard.json`](patch_debt_dashboard.json) for full JSON (`counts_by_category` must be non-empty for CI smoke).
