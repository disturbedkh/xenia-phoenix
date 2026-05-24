# Tier model

Phoenix organizes work into tiers. **Tier 1 is the north star**; Tier 0 proves the measurement pipeline; Tier 2 (patches) is debt to shrink.

## Tier table

| Tier | Description | Phoenix policy |
|------|-------------|----------------|
| **0** | PC-only differential testing, CI, build hygiene | Always green. CI gate. |
| **1** | Native 360 accuracy: VMX128, EDRAM/Xenos, XMA2, kernel | **Primary target.** |
| **2** | Per-game `game-patches` | Inventory + retire C/D categories. |
| **A** | HDMI capture-card ground truth | Optional; after Tier 0/1 saturate. |
| **B** | Modded-console / homebrew captures | Deferred until A exhausted. |
| **C** | Devkit / PIX-for-Xbox-360 | Out of scope (unobtainable). |

## Tier 1 sub-phases

Tier 1 splits into **PC** (no gameplay required) and **Gameplay** (retail titles).

### Tier 1-PC — complete (x64 Windows, 2026-05-16)

| Criterion | Status |
|-----------|--------|
| vmx128-fuzz ≥1M iterations, zero divergences | Green (`run_vmx128_full_sweep.ps1`) |
| GPU replay zero RTV/ROV drift (synthetic corpus) | Green |
| Patch debt inventory committed | Green (`patch_debt_dashboard_data.json`) |
| Kernel stub inventory + tracing infra | Green |
| CI `tier0-differential` on PR | Green |

### Tier 1-Gameplay — in progress

Finishes Tier 1 per mission definition:

1. Smoke roster (≥5 owned titles) — [../metacache/plan/60_smoke_titles.md](../metacache/plan/60_smoke_titles.md)
2. Per-title ~5 min captures + stub JSONL drain
3. Retire category **C** and **D** patches linked to smoke set
4. Retail GPU traces where legal
5. New title boots without a new patch entry

Checklist: [../metacache/plan/61_tier1_gameplay_gate.md](../metacache/plan/61_tier1_gameplay_gate.md)

## Definition of done — Tier 1 minimally achieved

All five must hold:

1. **VMX128:** `vmx128-fuzz` runs ≥1M random vectors per opcode with zero divergences in CI.
2. **GPU:** `tools/gpu_replay_ci/run.py` reports zero hash drift on the curated corpus (RTV and ROV).
3. **Kernel:** Stub-hit JSONL is empty for the smoke-title set (no unimplemented kernel calls in a 5-minute session).
4. **Patches:** Dashboard shows categories C and D at zero; only A/B remain.
5. **Generalization:** A randomly chosen untested title boots to gameplay without a new patch.

Full wording: [../metacache/plan/00_mission.md](../metacache/plan/00_mission.md)

## Tier 0 MVPs (reference)

| MVP | Output |
|-----|--------|
| MVP0 | Reproducible build — `docs/TIER0_BUILD.md`, `tools/tier0/bootstrap.ps1` |
| MVP1 | VMX128 fuzzer + guest PPC harness |
| MVP2 | GPU trace replay CI |
| MVP3 | Patch-debt dashboard |
| MVP4 | Kernel stub-trace JSONL |

See [../xenia-phoenix-src/docs/TIER0_README.md](../xenia-phoenix-src/docs/TIER0_README.md).

## Deeper detail (agents)

- Roadmap: [../metacache/plan/20_tier_roadmap.md](../metacache/plan/20_tier_roadmap.md)
- Gameplay execution: [../metacache/plan/62_gameplay_execution.md](../metacache/plan/62_gameplay_execution.md)
- Debt ledger: [../metacache/plan/30_debt_ledger.md](../metacache/plan/30_debt_ledger.md)
