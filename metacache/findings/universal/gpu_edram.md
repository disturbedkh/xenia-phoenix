---
area: GPU
last_verified: 2026-05-17
bf2_roadmap: ../../plan/bf2_gpu_roadmap.md
---

# Universal: GPU / EDRAM / depth

Parent RE: [20_xenos_gpu.md](../../re/20_xenos_gpu.md). Proof harness: `tools/gpu_replay_ci/`, [agents/gpu_edram.md](../../agents/gpu_edram.md).

---

## U-GPU-001: Host depth lost after EDRAM reload

**Status:** fix-candidate (2026-05-17) — v3 landed; v2 mismatch probe **0**; **visual verify failed** on Phoenix (flicker persists). Not `fix-landed` until [gpu_fix_bar.md](../../dev/gpu_fix_bar.md) visual pass.

### Pattern

Game **evicts** depth from EDRAM to RAM and later **reloads** it (possibly different `depth_base` or after another surface overwrote tiles). Emulator keeps **host float32** sidecar (`host_depth_render_target_*`) out of sync with guest **20e4** in EDRAM.

### Symptoms

- Interleaved stripes / patches of drawn vs undrawn geometry
- **Black polygon flicker** on terrain (BF2)
- Often fixed by `depth_float24_convert_in_pixel_shader=true` (forces 20e4 at PS — **not** a root fix)

### Detection

- Cvar description in `render_target_cache.cc` (`depth_float24_convert_in_pixel_shader`)
- F4 trace spanning resolve + redraw; look for ownership change before bad frame
- Probe: `ownership_change_count`, `edram_transfer_count`, `host_depth_store_count`, `host_depth_transfer_mismatch_count`
- **Observability v2:** invariant `DepthHostSidecarStale` / `HostDepthTransferMismatch` in `telemetry/{tid}_events.jsonl`; live counters `obs_*` on `/snapshot` — [observability.md](../../dev/observability.md)

### Fix direction

- Correct `ChangeOwnership` / `IsOwnedBy` when depth base moves but content continues
- Ensure `kColorAndHostDepthToDepth` / `kDepthAndHostDepthToDepth` transfers retain host depth when `stored_f24 == to_f24(stored_host)`
- `host_depth_store_*` on evict paths

### Fix landed (candidate)

1. **v2** — `ChangeOwnership`: keep host-depth source when `transfer_host_depth_source == transfer_source`; `IsOwnedBy` resync when host sidecar key ≠ depth key.
2. **v3** — `host_depth_guest_edram_stale` on ownership ranges; `NotifyGuestDepthEdramTilesTouched` from EDRAM UAV writes and depth surface rebind; allow **depth self-transfer** (`source == dest`) via `kDepthAndHostDepthToDepth` when stale. Cvar: `depth_resync_on_guest_edram_touch` (default true).

**Verify:** [bf2_user_verify.md](../../dev/bf2_user_verify.md) — target: flicker gone on defaults (no `PHOENIX_BF2_GFX_WORKAROUND`). **2026-05-17:** mismatch 0 but flicker **still present**; community path works on Edge ([G-454107DB-010](../games/454107db.md#g-454107db-010-edge-kortul-parity--fork-delta)). Mismatch 0 is necessary but not sufficient.

**RE note:** BF2 may reload depth without `ChangeOwnership` / UAV paths v3 hooks — see [bf2_gpu_roadmap.md](../../plan/bf2_gpu_roadmap.md) Track 4.

### Games exhibiting

| Game | Finding |
|------|---------|
| BF2 `454107DB` | [G-454107DB-003](../games/454107db.md#g-454107db-003-ground-black-polygon-flicker) |

**References:** [xenia-canary compat #9](https://github.com/xenia-canary/game-compatibility/issues/9#issuecomment-3997637068)

---

## U-GPU-002: Invalid GPU upload range vs lazy mapping

**Status:** open (mitigated in Phoenix fork; **material on Edge** for BF2)

### Pattern

`QueryRangeAccess` reports `kNoAccess` at upload range start/end while guest memory is still valid for `TranslatePhysical` after `MakeRangeValid` (lazy map / timing).

### Symptoms

- `Invalid upload range for GPU` spam in crash log
- Aborted uploads → stale GPU memory → widespread corruption if batch aborts

### Fork behavior (BF2 kortul triangulation)

| `gpu_allow_invalid_upload_range` | Edge (`d3d12_shared_memory.cc`) | Phoenix |
|----------------------------------|----------------------------------|---------|
| `false` (default) | **Aborts** entire `UploadRanges` batch | **Warns + continues** each range |
| `true` | Permissive (community BF2 workaround) | Often redundant; same cvar exists |

Community BF2 fix pairs this with `depth_float24_convert_in_pixel_shader` on **Edge** ([compat #9](https://github.com/xenia-canary/game-compatibility/issues/9#issuecomment-3997637068)). On Phoenix, upload continue may already apply — parity test still required ([bf2_gpu_roadmap.md](../../plan/bf2_gpu_roadmap.md) Track 3).

### Detection

- `phoenixctl log scan` → `upload_range_errors`
- Probe `gpu.upload_range_error_count` / events `upload_range_error`
- **Observability v2:** invariant `GpuUploadRangeError` in events JSONL; `obs_gpu_upload_range_error_count` on snapshot

### Fix direction

- Do not abort entire `UploadRanges` batch on false positive (Phoenix: warn + continue — landed)
- Upstream-aligned: only reject when memcpy would fault
- `gpu_allow_invalid_upload_range=true` is blunt workaround (critical on Edge; verify on Phoenix)

### Games exhibiting

| Game | Finding |
|------|---------|
| BF2 `454107DB` | [G-454107DB-003](../games/454107db.md) (correlated) |

---

## U-GPU-003: Float24 PS workaround side effects

**Status:** open

### Pattern

Forcing `depth_float24_convert_in_pixel_shader` aligns depth tests but changes PS behavior (MSAA sample frequency, viewport bounds per cvar docs).

### Symptoms

- **Camera halo / dark blob** following player (BF2 with community workaround)
- Extra GPU cost; depth compression disabled

### Detection

- A/B: workaround on vs `PHOENIX_BF2_GFX_LEGACY=1`
- Halo unchanged when toggling `readback_resolve` alone (BF2)

### Fix direction

- Resolve U-GPU-001 so cvar not needed
- If halo persists: audit viewport float24 conversion noted in cvar help text

### Games exhibiting

| Game | Finding |
|------|---------|
| BF2 `454107DB` | [G-454107DB-004](../games/454107db.md) |

---

## U-GPU-004: RTV vs ROV resolve drift

**Status:** confirmed (harness); per-title traces pending

### Pattern

Same `.xtr` produces different pixel hash on D3D12 RTV path vs ROV path.

### Symptoms

Title-specific corruption may exist on only one path.

### Detection

`run_gpu_replay.ps1` / `tools/gpu_replay_ci/run.py` strict mode

### Fix direction

Drift log in [20_xenos_gpu.md](../../re/20_xenos_gpu.md); fix `d3d12_render_target_cache.cc` TODOs with trace proof

### Games exhibiting

| Game | Finding |
|------|---------|
| *Synthetic corpus only* | Phase 1.2 bootstrap green |

---

## U-GPU-005: Uninstrumented launch invalidates GPU triage

**Status:** fix-landed (process + guard)

### Pattern

Default exe flags (`readback_resolve=none`, scratch trace paths, `hid=any`) make graphics look universally broken independent of game logic.

### Detection

CONFIG DUMP in crash log; `phoenixctl launch` → `launch_guard`

### Games exhibiting

| Game | Finding |
|------|---------|
| BF2 | [G-454107DB-002](../games/454107db.md) |
