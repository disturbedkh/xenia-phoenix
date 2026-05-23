# BF2 U-GPU-001 — user verify sessions

## 2026-05-17 — Instrumented session (v2/v3 build)

**Launch:** `bf2_gpu_session.ps1 -Launch` / `launch_bf2_triage` (Debug `xenia_canary.exe`).

| Check | Result |
|-------|--------|
| `launch.readback_resolve` | `fast` |
| `hid` | `xinput` |
| `host_depth_transfer_mismatch_count` | **0** (was **263** on `454107DB_7687` baseline) |
| Ground black-polygon flicker (defaults, no `PHOENIX_BF2_GFX_WORKAROUND`) | **Still present** |
| Flicker with `PHOENIX_BF2_GFX_WORKAROUND=1` | **Still present** |

**Takeaway:** Mismatch probe green is **necessary but not sufficient**. Native U-GPU-001 v2/v3 does not close G-454107DB-003 visually. Next: [kortul parity on Phoenix](../plan/bf2_gpu_roadmap.md#track-3--kortul-parity-on-phoenix-next) and RE on reload paths without ownership transfer.

Record PostOnly summary in `telemetry/454107db_obs_summary.json`; check `classification.finding_ids` for `U-GPU-001` / `G-454107DB-003`.

---

## Standard verify procedure (after any host-depth patch)

After `bf2_native_engineering.ps1 -Build` (Debug `xenia_canary.exe`):

```powershell
cd xenia-phoenix-src
$env:PHOENIX_DEBUG_PORT = "8765"
# Do NOT set PHOENIX_BF2_GFX_WORKAROUND at boot.

powershell -File tools/tier0/bf2_gpu_session.ps1 -Launch
# ~2 min: zone_01a, stand on flickering ground (same spot as 6004 capture)

phoenixctl probe snapshot
# Expect: launch.readback_resolve == fast
#         gpu.host_depth_transfer_mismatch_count == 0

# quit game
powershell -File tools/tier0/bf2_gpu_session.ps1 -PostOnly
```

**Pass (native fix bar):** no ground black-polygon flicker on **defaults** (no gfx workaround).

**Toggle:** `depth_resync_on_guest_edram_touch=false` to A/B v3 off.

**If flicker remains:** one-shot `--depth_resync_each_depth_draw=true` (forces sidecar self-transfer every depth draw; heavy). If that fixes flicker, reload detection needs a better hook than EDRAM UAV touch.

---

## Kortul parity (Edge community workaround on Phoenix)

Reference: [game-compat #9 — kortul, Mar 2026](https://github.com/xenia-canary/game-compatibility/issues/9#issuecomment-3997637068) (tested on **Xenia Edge** `707052a2f`).

```powershell
cd xenia-phoenix-src
$env:PHOENIX_DEBUG_PORT = "8765"

# Option A: explicit flags (isolate from native resync)
# Add to launch script or manual exe line:
#   --depth_float24_convert_in_pixel_shader=true
#   --gpu_allow_invalid_upload_range=true
#   --depth_resync_on_guest_edram_touch=false

# Option B: in-game workaround env (see bf2_gpu_session -GfxWorkaround)
$env:PHOENIX_BF2_GFX_WORKAROUND = "1"
powershell -File tools/tier0/bf2_gpu_session.ps1 -Launch -GfxWorkaround
```

| Check | Edge (reported) | Phoenix (fill after run) |
|-------|-----------------|--------------------------|
| Flicker gone | Yes | _pending_ |
| Camera halo | Yes (G-454107DB-004) | _pending_ |
| CONFIG DUMP shows float24 PS | — | _verify_ |

**Pass:** flicker gone (halo acceptable under workaround track). **Fail on Phoenix but pass on Edge:** fork delta — see [bf2_gpu_roadmap.md](../plan/bf2_gpu_roadmap.md#edge-vs-canary-vs-phoenix-re-triangulation).

Record counters in [bf2_probe_baseline_workaround.json](bf2_probe_baseline_workaround.json) when run.

Baseline before native patches: [bf2_probe_baseline_7687.json](bf2_probe_baseline_7687.json).
