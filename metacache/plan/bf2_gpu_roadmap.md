# BF2 GPU roadmap (`454107DB`)

## Parked (2026-05-17)

**Active Tier 1 work:** [edge_port_queue.md](edge_port_queue.md), [60_smoke_titles.md](60_smoke_titles.md) roster captures, [30_debt_ledger.md](30_debt_ledger.md) C-patch retirement.

**No new BF2 instrumentation, patches, or play sessions** until all of:

1. Smoke roster captured for **≥3** titles (see [62_gameplay_execution.md](62_gameplay_execution.md) §2), **and**
2. **≥1** smoke-linked category-C patch retired, **and**
3. At least one of: (a) another smoke title shows U-GPU-001 ground-flicker symptoms, (b) an Edge class-A port maps to U-GPU-001 depth/upload, (c) Tier A capture-card ground truth is scheduled.

Tracks 0–5 below remain reference only.

---

**Last updated:** 2026-05-17  
**Compat:** [xenia-canary/game-compatibility #9](https://github.com/xenia-canary/game-compatibility/issues/9)  
**Game finding:** [findings/games/454107db.md](../findings/games/454107db.md)  
**Universal patterns:** [U-GPU-001](../findings/universal/gpu_edram.md#u-gpu-001-host-depth-lost-after-edram-reload), [U-GPU-002](../findings/universal/gpu_edram.md#u-gpu-002-invalid-gpu-upload-range-vs-lazy-mapping), [U-GPU-003](../findings/universal/gpu_edram.md#u-gpu-003-float24-ps-workaround-side-effects)

## Current bar

| Gate | Status |
|------|--------|
| Kernel stubs drained | **GREEN** |
| Instrumented launch (`readback_resolve=fast`, `hid=xinput`) | **GREEN** |
| Legal in-game `.xtr` at repro | **GREEN** — `454107DB_6004.xtr`, `454107DB_7687.xtr` |
| `phoenixctl trace validate` | **GREEN** |
| `host_depth_transfer_mismatch_count == 0` (instrumented) | **GREEN** (v2+) |
| Ground flicker gone on **defaults** (no gfx workaround) | **RED** — user verify 2026-05-17 |
| Community workaround (float24 PS + permissive upload) | **GREEN on Edge** — user reconfirmed 2026-05-16 ([G-454107DB-011](../findings/games/454107db.md#g-454107db-011-edge-user-playtest--workaround--remaining-gfx-2026-05-16)); **unconfirmed on Phoenix** |
| Secondary shadow / halo with workaround | **YELLOW** — still present on Edge after flicker fix (G-454107DB-004) |
| Boot / load FMV + A/V to menu and mission load | **GREEN on Edge** (G-454107DB-012) |
| In-mission audio + crash | **RED** on Edge (G-454107DB-013) |
| Rainbow sky rays | **RED** on Edge; not fixed by kortul cvars (G-454107DB-014) |
| Headless `trace-dump` on BF2 `.xtr` | **AV** `0xC0000005` — documented; not a fix gate |

**Conclusion:** Probe counters and native U-GPU-001 patches (v2/v3) are necessary instrumentation but **do not close** G-454107DB-003 visually. Next work splits into **kortul parity on Phoenix** (community path) and **deeper RE** (reload paths without ownership transfer, CPU depth tile uploads).

---

## Symptom map

| Symptom | Finding | Primary pattern |
|---------|---------|-----------------|
| Black polygon flicker on terrain (standing still) | G-454107DB-003 | U-GPU-001 — host float32 sidecar vs guest 20e4 after EDRAM reload |
| Camera halo / secondary shadow with workaround | G-454107DB-004 | U-GPU-003 — float24 PS side effect |
| Rainbow rays from sky (intermittent) | G-454107DB-014 | TBD — separate from U-GPU-001 / kortul |
| Boot / load FMV broken | G-454107DB-012 | **Fixed on Edge** (2026-05-16 user) |
| In-mission audio loss + crash | G-454107DB-013 | APU / guest stability — not U-GPU-001 |
| Invalid upload range spam | G-454107DB-006 / U-GPU-002 | Lazy map timing; batch abort on Edge |
| Mis-instrumented “always broken” gfx | G-454107DB-002 | U-GPU-005 — process (resolved) |

---

## Edge vs Canary vs Phoenix (RE triangulation)

Community verification ([compat #9 comment, Mar 4 2026](https://github.com/xenia-canary/game-compatibility/issues/9#issuecomment-3997637068)) was on **Xenia Edge** commit `707052a2f`, **not** Canary/Phoenix.

### What fixes flicker on Edge (kortul)

- `depth_float24_convert_in_pixel_shader = true`
- `gpu_allow_invalid_upload_range = true`
- Trade-off: **camera halo** (G-454107DB-004)

### What did **not** affect halo (same report)

`readback_resolve`, render target path (RTV/ROV), `clear_memory_page_state`, draw scale, Vulkan vs D3D12, `depth_transfer_not_equal_test`, `mrt_edram_used_range_clamp_to_min`.

### Fork deltas (material for Phoenix)

| Topic | Edge | Phoenix (`xenia-phoenix-src`) |
|-------|------|-------------------------------|
| `gpu_allow_invalid_upload_range=false` | **Aborts** entire `UploadRanges` batch (`d3d12_shared_memory.cc`) | **Warns + continues** upload (cvar often redundant) |
| U-GPU-001 native patches | Not present — still clears host depth when `transfer_host_depth_source == transfer_source` | v2+v3 in `render_target_cache.cc` |
| `depth_float24` PS path | Same family as Canary (`pipeline_cache`, `IsHostDepthEncodingDifferent`) | Same; float24 PS **disables** host sidecar path when on |
| Async D3D12 pipeline | May skip draw if pipeline not ready | `EnsureD3D12PipelineReady` blocks/creates |
| Windows default `hid` | `sdl` | `xinput` (2026-05-17; config file overrides) |

**Inference:** Default path = broken host float32 DSV / EDRAM ownership. Community workaround = PS float24 + (on Edge) permissive uploads. Phoenix native fixes and community workaround are **orthogonal** — fixing mismatch count ≠ fixing flicker if reload happens without the paths we hook.

Reference tree: [workspace/00_monorepo_pointers.md](../workspace/00_monorepo_pointers.md) → `Xenia-Edge/`.

---

## Tracks

### Track 0 — Live probe at repro (ongoing)

Stand on flickering ground; poll counters without F4.

```powershell
cd xenia-phoenix-src
$env:PHOENIX_DEBUG_PORT = "8765"
powershell -File tools/tier0/bf2_gpu_session.ps1 -Launch
powershell -File tools/tier0/probe_bf2_live.ps1
```

See [dev/observability.md](../dev/observability.md).

### Track 1 — Proof loop / corpus (**done**)

- `bf2_gpu_session.ps1`, `bf2_native_engineering.ps1 -All`
- Corpus symlink: `tests/gpu_traces/454107DB_6004.xtr`
- Baseline: [dev/bf2_probe_baseline_7687.json](../dev/bf2_probe_baseline_7687.json)
- Finding: G-454107DB-008

### Track 2 — Native U-GPU-001 (**landed, visual fail**)

Patches in `render_target_cache.cc` / `.h`:

1. **v1** — Keep `transfer_host_depth_source` when `== transfer_source`
2. **v2** — `IsOwnedBy` when host sidecar key ≠ depth key; mismatch probe
3. **v3** — `host_depth_guest_edram_stale`, `NotifyGuestDepthEdramTilesTouched`, depth self-transfer, `depth_resync_on_guest_edram_touch` (default true)

**User verify (2026-05-17):** `host_depth_transfer_mismatch_count: 0`, flicker **still present** with and without `PHOENIX_BF2_GFX_WORKAROUND=1`.

Finding: G-454107DB-009 → status **open** (native path insufficient). U-GPU-001 remains **fix-candidate** for counter/RE value, not **resolved-native**.

Diagnostics: `depth_resync_on_guest_edram_touch=false` (A/B v3); `--depth_resync_each_depth_draw=true` (heavy).

### Track 3 — Kortul parity on Phoenix (**next**)

**Edge baseline (user 2026-05-16):** workaround still clears ground flicker; shadow remains; boot/load A/V good; mission audio/crash and sky rays **not** in scope of kortul cvars.

Confirm Phoenix matches Edge community behavior before blaming fork drift.

```powershell
cd xenia-phoenix-src
# Explicit community flags; disable native resync to isolate PS path:
xenia_canary.exe "<game>" `
  --depth_float24_convert_in_pixel_shader=true `
  --gpu_allow_invalid_upload_range=true `
  --depth_resync_on_guest_edram_touch=false `
  --readback_resolve=fast --hid=xinput
```

**Pass:** flicker gone (expect halo). **Fail:** verify CONFIG DUMP shows float24 PS active; diff `d3d12_command_processor.cc` / `pipeline_cache` vs Edge; check async compile does not skip modified shaders.

Or: `bf2_gpu_session.ps1 -Launch -GfxWorkaround` with `depth_resync_on_guest_edram_touch=false` if script exposes it.

Finding: [G-454107DB-010](../findings/games/454107db.md#g-454107db-010-edge-kortul-parity--fork-delta).

### Track 4 — Fork / upload RE (**parallel**)

- A/B `gpu_allow_invalid_upload_range` on Phoenix (likely noop if continue-upload already on)
- Trace CPU uploads touching depth EDRAM tiles (not only UAV ownership path)
- F4 span: reload **without** `ChangeOwnership` — v3 hooks may never run

### Track 5 — Close gfx bar (**blocked**)

Per [dev/gpu_fix_bar.md](../dev/gpu_fix_bar.md):

1. Repro without title launch cvars
2. Visual pass on defaults **or** documented native fix + halo acceptable
3. Drift row in [re/20_xenos_gpu.md](../re/20_xenos_gpu.md)
4. U-GPU-* → `fix-landed` only after visual + replay policy met

Until Track 3 passes or Track 2 finds the real reload hook, smoke status stays **workaround** ([60_smoke_titles.md](60_smoke_titles.md)).

---

## Commands (quick reference)

```powershell
cd xenia-phoenix-src
powershell -File tools/tier0/bf2_native_engineering.ps1 -All
powershell -File tools/tier0/bf2_gpu_session.ps1 -Launch
# play → F4 at repro → quit
powershell -File tools/tier0/bf2_gpu_session.ps1 -PostOnly -Triage
phoenixctl guard check --title-id 454107DB --json
powershell -File tools/tier0/play_bf2.ps1   # casual play, xinput
```

User verify checklist: [dev/bf2_user_verify.md](../dev/bf2_user_verify.md).

---

## Artifacts

| Path | Role |
|------|------|
| `telemetry/454107db_gpu_trace/*.xtr` | Retail repro captures |
| `telemetry/454107db_obs_summary.json` | Post-session classification |
| `metacache/dev/bf2_probe_baseline_7687.json` | Pre-fix mismatch baseline (263) |
| `tests/gpu_traces/golden/last_report.json` | Headless replay outcome |
| `src/xenia/gpu/render_target_cache.cc` | U-GPU-001 native |
| `src/xenia/gpu/d3d12/d3d12_shared_memory.cc` | U-GPU-002 Phoenix vs Edge |
