# Smoke title roster (Phase 1.3)

Curated titles for **5-minute** play sessions. Telemetry writes to `xenia-phoenix-src/telemetry/` (gitignored). Do not commit ISO paths in public forks.

## User checklist (blocks first gameplay capture)

- [x] Add **≥5** owned titles below (roster slots filled; BF2 parked separately).
- [ ] **You:** copy [smoke_roster_local.example.toml](../dev/smoke_roster_local.example.toml) → `xenia-phoenix-src/telemetry/smoke_roster_local.toml` and set each `game_path` (do not commit).
- [x] Patch triage: `list_smoke_patches.py` per ID (see [first_c_patch_retirement.md](../dev/first_c_patch_retirement.md)).
- [ ] **You:** `powershell -File tools/tier0/run_smoke_roster.ps1` after local TOML exists; target **empty** stub + XMA JSONL.

| title_id | Name | Boot command / notes | Linux boot | Windows ARM64 (WoA) | macOS ARM64 | 5-min scenario | Patches (A/B/C) | stub_log | pcm_baseline | xma_div_log | gpu_xtr | notes |
|----------|------|----------------------|------------|---------------------|----------------|-----------------|----------|--------------|-------------|---------|-------|
| `4D5307D1` | Project Gotham Racing 3 | `run_smoke_capture.ps1 -TitleId 4D5307D1 -GamePath <your.xex>` | — | — | — | 5 min menu / short drive | `list_smoke_patches.py` | `4d5307d1_stubs.jsonl` | `4d5307d1_pcm.jsonl` | `4d5307d1_xma.jsonl` | optional | **ready** — fill GamePath locally |
| `4D5307EA` | Forza Motorsport 2 | same pattern | — | — | — | 5 min menu | C/D | `4d5307ea_*` | | | | pending |
| `4D5309B1` | Halo CE Anniversary | same pattern | — | — | — | 5 min menu | C/D | `4d5309b1_*` | | | | pending |
| `58410955` | Banjo-Tooie | same pattern | — | — | — | 5 min gameplay | C/D | `58410955_*` | | | | pending |
| `4D53082D` | Gears of War 2 | same pattern | — | — | — | 5 min menu | C/D | `4d53082d_*` | | | | pending |
| `454107DB` | Battlefield 2: Modern Combat | **parked** — [bf2_gpu_roadmap.md](bf2_gpu_roadmap.md) § Parked | — | — | — | (frozen) | 0 C/D | `454107db_stubs.jsonl` | `454107db_pcm_baseline.jsonl` | `454107db_xma.jsonl` | `454107db_gpu_trace/` | **parked (workaround)**; revisit after smoke + 1 C-patch retired |
| | | | | | | | | | | | | |

## Android (Phase 3 QA)

When a legally owned title boots on device via SAF, record the launcher `target` URI pattern and result in [android_device_qa.md](../../xenia-phoenix-src/docs/android_device_qa.md). No extra table column until first green device boot.

## Capture protocol

From `xenia-phoenix-src` build with Release binaries:

Single entry point (kernel + audio in one session):

```powershell
tools/tier0/run_smoke_capture.ps1 -TitleId 0xTTTTTTTT -GamePath "path\to\default.xex"
```

Or manually (Windows):

```text
xenia-canary.exe <your_default.xex> `
  --kernel_stub_hit_log=telemetry/TTTTTTTT_stubs.jsonl `
  --apu_xma_divergence_log=telemetry/TTTTTTTT_xma.jsonl `
  --apu_pcm_hash_log=telemetry/TTTTTTTT_pcm.jsonl
```

Linux (from `xenia-phoenix-src`, Release build):

```bash
./scripts/xenia-phoenix-linux.sh /path/to/default.xex \
  --kernel_stub_hit_log=telemetry/TTTTTTTT_stubs.jsonl \
  --apu_xma_divergence_log=telemetry/TTTTTTTT_xma.jsonl \
  --apu_pcm_hash_log=telemetry/TTTTTTTT_pcm.jsonl
```

Compare Linux vs Windows stub JSONL for the same `title_id`; platform-specific hits go to [30_debt_ledger.md](30_debt_ledger.md).

1. Play the scenario in the table for **5 minutes** (or **30s** for first PCM baseline).
2. Quit cleanly.
3. Aggregate stubs: `python tools/tier0/aggregate_stub_hits.py telemetry/TTTTTTTT_stubs.jsonl`
4. Aggregate XMA: `python tools/tier0/aggregate_xma_divergences.py telemetry/TTTTTTTT_xma.jsonl`
5. Compare PCM: `python tools/tier0/compare_pcm_hash_log.py telemetry/TTTTTTTT_pcm.jsonl telemetry/TTTTTTTT_30s_pcm_baseline.jsonl`
6. Implement top hits (Phases 1.3–1.4); re-run until stub and XMA JSONL are **empty** and PCM hash is **stable**.

## Patch-debt cross-check

For category **C** patches on a title, capture stub log while **patch disabled** and map `(module, export)` hits to [30_debt_ledger.md](30_debt_ledger.md) retirement steps.
