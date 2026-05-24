# Patch debt

Phoenix tracks every entry in `game-patches/*.patch.toml`. **Goal: shrink categories C and D to zero** by fixing the emulator, not by adding patches.

## Categories

| Cat | Meaning | Policy |
|-----|---------|--------|
| **A** | Legitimate game-side patch (cheat, language pack, user-requested unlock) | Keep. Document. |
| **B** | Retail hardware bug or post-release fix | Keep. Document. |
| **C** | Workaround for an **emulator bug** — on real 360 the original code works | **Retire** via emulator fix |
| **D** | Unknown hack; root cause unclear | Triage → fix or reclassify |

## Inventory (Tier 1-PC snapshot)

Heuristic categorization (~2026-05-16):

| Category | Count (approx.) |
|----------|-----------------|
| A | 83 |
| B | 536 |
| C | 988 |
| D | 80 |

Source JSON: [../xenia-phoenix-src/docs/patch_debt_dashboard_data.json](../xenia-phoenix-src/docs/patch_debt_dashboard_data.json)

**Policy during Tier 1-Gameplay:** do not delete patches until gameplay repro confirms retirement — see gameplay gate below.

## Generate / refresh dashboard

```powershell
cd xenia-phoenix-src
$env:GAME_PATCHES_ROOT = "..\game-patches"   # or in-tree game-patches
python tools/tier0/categorize_patches.py
```

Human-readable dashboard: [../xenia-phoenix-src/docs/patch_debt_dashboard.md](../xenia-phoenix-src/docs/patch_debt_dashboard.md)

## Retirement procedure (category C)

1. Reproduce without the patch — capture failure mode.
2. Capture telemetry: `--kernel_stub_hit_log=`, GPU trace at failing scene.
3. Fix root cause (kernel shim, VMX128, GPU EDRAM, XMA2, etc.).
4. Land emulator fix; **delete** patch entry.
5. Log retirement in agent session log.

Likely buckets: unimplemented kernel function, VMX128 divergence, GPU resolve bug, audio decode bug.

## Smoke-linked patches

During Tier 1-Gameplay, list patches for your smoke roster:

```powershell
python tools/tier0/list_smoke_patches.py --title-id <ID1> <ID2> ...
```

## Deeper detail (agents)

- Ledger: [../metacache/plan/30_debt_ledger.md](../metacache/plan/30_debt_ledger.md)
- Gameplay gate §3: [../metacache/plan/61_tier1_gameplay_gate.md](../metacache/plan/61_tier1_gameplay_gate.md)
- First C-patch retirement walkthrough: [../metacache/dev/first_c_patch_retirement.md](../metacache/dev/first_c_patch_retirement.md)
