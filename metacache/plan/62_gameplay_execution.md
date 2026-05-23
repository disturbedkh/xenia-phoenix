# Tier 1 gameplay execution tracker

Prerequisites: **done** (Tier 1-PC). Procedure: [61_tier1_gameplay_gate.md](61_tier1_gameplay_gate.md).

## Agent / infra (complete)

- [x] Tier 1-PC differential gates green (vmx128 1M local, synthetic GPU, XMA fixtures, stub infra).
- [x] PR CI `tier0-differential` wired in Orchestrator.
- [x] `telemetry/` layout + `list_smoke_patches.py`.
- [x] Docs synced (roadmap, gameplay gate, Q6).

## User + runtime (pending)

### §1 Smoke roster

- [x] ≥5 titles in [60_smoke_titles.md](60_smoke_titles.md) (BF2 parked; five active slots).
- [x] `list_smoke_patches.py` run — summaries in [first_c_patch_retirement.md](../dev/first_c_patch_retirement.md).
- [ ] **User:** `telemetry/smoke_roster_local.toml` from [smoke_roster_local.example.toml](../dev/smoke_roster_local.example.toml).

### §2 Captures (per title)

- [ ] **User:** `run_smoke_roster.ps1` (wraps `run_smoke_capture.ps1` × 5).
- [ ] Stub JSONL empty (or baselined).
- [ ] XMA JSONL empty.
- [ ] PCM hash stable vs 30s baseline.

### §3 Patch retirement

- [ ] First smoke-linked **C** patch reproduced without patch → fix → delete (queued: Gears 2 **Black Shading Fix**).
- [x] Track queued in [30_debt_ledger.md](30_debt_ledger.md) + [first_c_patch_retirement.md](../dev/first_c_patch_retirement.md).

### §4 Retail GPU

- [ ] ≥1 legal `.xtr` capture + replay green (BF2 corpus exists but title parked).

### §5 New title

- [ ] One title outside roster/patches boots without new patch (candidate: Forza 2 — 0 C/D patches).

### Edge harvest (parallel)

- [x] [edge_port_queue.md](edge_port_queue.md) first pass.
- [x] Class-A ports EDGE-PORT-A-1, A-2 (`audio_media_player.cc`).

### §7 Sign-off

- [ ] Update [20_tier_roadmap.md](20_tier_roadmap.md) Tier 1-Gameplay → complete.
- [x] Session log entry (2026-05-17 Edge + smoke automation).

## CI first-green (maintainer)

- [ ] GitHub: `tier0-differential` green on a PR.
- [ ] Optional: `workflow_dispatch` → `tier0-windows.yml` or `vmx128-1m-weekly.yml`.
