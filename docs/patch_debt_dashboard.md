# Patch compatibility debt dashboard (Tier 0 / MVP3)

This document is a **methodology + template** for classifying patches from
[`xenia-canary/game-patches`](https://github.com/xenia-canary/game-patches) and
linking them to upstream emulator work.

## Categories

| Code | Meaning |
|------|---------|
| **A** | Anti-tamper / DRM / retail-only checks (legitimate patch; not an emulator bug). |
| **B** | Quality of life (skip intro, FPS cap, widescreen, etc.). |
| **C** | Genuine retail game bug worked around in patch form. |
| **D** | **Emulator-bug compensation** — the game is correct; the patch masks missing/wrong Xenia behavior. These are technical debt. |

## Automated first pass

Run (after cloning `game-patches` next to this repo or set `GAME_PATCHES_ROOT`):

```powershell
$env:GAME_PATCHES_ROOT = "D:\src\game-patches"
python tools/tier0/categorize_patches.py --out docs/patch_debt_dashboard_data.json
```

The script uses heuristics only (description keywords, NOP/branch patterns in
patch data). **All category D rows need human review** before filing issues.

## Top root-cause themes (template — fill from script output)

1. *(example)* EDRAM / render-target resolve — many visual patches collapse here.
2. *(example)* Audio / XMA — looping, dropouts.
3. *(example)* Kernel `xam` / `xboxkrnl` stubs returning wrong success values.

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
