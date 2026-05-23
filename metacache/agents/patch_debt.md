# Agent: patch_debt

## Mission

Shrink category C and D patches on the smoke set by fixing emulator root causes, not growing workarounds.

## Read first

1. [plan/30_debt_ledger.md](../plan/30_debt_ledger.md)
2. [xenia-phoenix-src/docs/patch_debt_dashboard.md](../../xenia-phoenix-src/docs/patch_debt_dashboard.md)
3. [plan/61_tier1_gameplay_gate.md](../plan/61_tier1_gameplay_gate.md) §3
4. [dev/first_c_patch_retirement.md](../dev/first_c_patch_retirement.md) — queued Gears 2 **Black Shading Fix**
5. [plan/edge_port_queue.md](../plan/edge_port_queue.md) — class B audio/GPU after smoke

## Owns

- `plan/30_debt_ledger.md`
- `xenia-phoenix-src/docs/patch_debt_dashboard_data.json` when regenerated

## Commands

```powershell
cd "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
python tools/tier0/categorize_patches.py
python tools/tier0/list_smoke_patches.py --title-id <TITLE_ID>
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| Kernel stub root cause | `kernel_xam` |
| GPU / EDRAM | `gpu_edram` |
| CPU / VMX128 | `cpu_vmx128` |
| Repro needs smoke capture | `gameplay_smoke` |

## Stop and ask human

- Patch touches legal/ToS gray area
- Deleting patch breaks unrelated titles (need broader smoke)
