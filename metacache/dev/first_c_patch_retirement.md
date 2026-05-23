# First smoke-linked C-patch retirement (queued)

**Status:** queued — waiting on `telemetry/smoke_stub_summary.json` from [run_smoke_roster.ps1](../../xenia-phoenix-src/tools/tier0/run_smoke_roster.ps1).

## Candidate (pre-smoke)

| Priority | Title | Patch | Category | Subsystem |
|----------|-------|-------|----------|-----------|
| 1 | `4D53082D` Gears of War 2 | **Black Shading Fix** | C | GPU (`d3d12_render_target_cache` / shading) |
| 2 | `58410955` Banjo-Tooie | **HD Shadows** | C | GPU |
| 3 | `4D5309B1` Halo CE A | 1280x720 Resolution | C | GPU (may be QoL — verify on hardware) |

Forza 2 (`4D5307EA`) has **0** C/D rows in dashboard — good §5 new-title boot candidate.

## Procedure ([30_debt_ledger.md](../plan/30_debt_ledger.md))

1. Run smoke capture with patch **enabled**; confirm stub/XMA baseline.
2. Disable **Black Shading Fix** in `game-patches/4D53082D - Gears of War 2 (TU6).patch.toml`.
3. Reproduce 60–120s; capture stub log + optional F4 GPU trace at black shading.
4. Map failure to top `(module, export)` from `smoke_stub_summary.json` or GPU invariants.
5. Land Phoenix fix (not a new title-only hack).
6. Delete patch row; log `C -> retired` in [30_debt_ledger.md](../plan/30_debt_ledger.md).

## Commands

```powershell
cd xenia-phoenix-src
python tools/tier0/list_smoke_patches.py --title-id 4D53082D
# After smoke_roster_local.toml:
powershell -File tools/tier0/run_smoke_roster.ps1
```

Edge audio ports (EDGE-PORT-A-1/A-2) may help unrelated APU issues but do not replace GPU fix for Black Shading.
