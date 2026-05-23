# Phase 1.2 GPU trace corpus

`kTraceFormatVersion` is **1** (see `src/xenia/gpu/trace_protocol.h`). Re-run
`tools/gpu_replay_ci/gen_phase12_fixture_xtr.py` after any trace format bump.

## Slots (coverage matrix)

Synthetic **event-only** fixtures (`phoenix_slot_*_min.xtr`, 56 bytes each) bootstrap
corpus presence and format validation. They close one frame via `kEvent`/`kSwap` without
PM4 execution. **RTV-vs-ROV PNG hashing** requires legal retail captures that exercise
resolves; use `tools/tier0/run_gpu_replay.ps1` on a machine where
`xenia-gpu-d3d12-trace-dump.exe` completes (local setup may crash headless — see
`last_report.json` `trace_dump_exit_code` when matched failures are recorded).
Replace slots per [README.md](README.md) as you obtain legal captures.

| Slot file | Intended real-world coverage | Fixture status |
|-----------|------------------------------|----------------|
| `phoenix_slot_a_depth_resolve_min.xtr` | Depth-heavy / depth-only resolve | Synthetic min |
| `phoenix_slot_b_msaa_color_min.xtr` | MSAA color resolve | Synthetic min |
| `phoenix_slot_c_msaa_depth_min.xtr` | MSAA depth resolve | Synthetic min |
| `phoenix_slot_d_tiled_color_resolve_min.xtr` | Color resolve to tiled texture | Synthetic min |
| `phoenix_slot_e_predicated_viewport_min.xtr` | Predicated tiling / viewport offset | Synthetic min |
| `phoenix_slot_f_memexport_stress_min.xtr` | memexport / UAV-like stress | Synthetic min |
| `phoenix_slot_g_depth_only_min.xtr` | Depth-only path | Synthetic min |
| `phoenix_slot_h_gamma_ramp_min.xtr` | Gamma / display path | Synthetic min |
| `phoenix_slot_i_bin_mask_min.xtr` | Bin mask / tiled binning | Synthetic min |
| `phoenix_slot_j_generic_clear_min.xtr` | Generic clear / simple frame | Synthetic min |

**Optional expansion:** add two more captures (11–12) for edge cases once you have
legal traces (e.g. separate 2x vs 4x MSAA titles).

## Retail (local symlink — not committed)

Legal captures stay under `telemetry/454107db_gpu_trace/`. Install into this
directory without copying ~90 MB files:

```powershell
powershell -File tools/tier0/bf2_native_engineering.ps1 -InstallCorpus -Include7687
```

| File | Repro | Format validate | trace-dump |
|------|-------|-----------------|------------|
| `454107DB_6004.xtr` | BF2 `zone_01a` ground flicker spot (primary) | Pass (2026-05-17) | Run `-Replay` to record |
| `454107DB_7687.xtr` | Same; probe baseline session (`mismatch=263`) | Pass | Optional A/B |
| `454107DB_3940.xtr` | Older capture | Pass | Known AV (`0xC0000005`) — do not gate fix on this file |

Baseline counters: [metacache/dev/bf2_probe_baseline_7687.json](../../../metacache/dev/bf2_probe_baseline_7687.json).
