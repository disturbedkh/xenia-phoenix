# Edge port queue

**Purpose:** Harvest low-risk fixes from [Xenia-Edge](../../Xenia-Edge/) into `xenia-phoenix-src` without disturbing Phoenix Tier 1-PC wins (vmx128, U-GPU instrumentation, `apu_trace`, stub logging, upload continue).

**Audit method:** Commit log scan + cherry-pick trial on `sync/edge-port-a-2026-05-23` (2026-05-23). Edge HEAD `6e02a106c`; Phoenix HEAD `466804473` (`canary_experimental`, Tier A+B + Tier S + follow-ups landed 2026-05-23). Prior audit: 2026-05-17 @ Edge `6101f13`.

**Scope:** `src/xenia/{apu,gpu,kernel,hid,base,cpu/ppc}` — excludes `cpu/backend/x64`, `cpu/backend/a64`, build/CI.

## Classes

| Class | Meaning | Action |
|-------|---------|--------|
| **A** | Self-contained Edge fix; no Phoenix conflict | Port now |
| **B** | Edge ahead; Phoenix diverged (needs diff + smoke) | Queue after smoke captures |
| **C** | Phoenix deliberate superset or different policy | Skip |
| **D** | Unclear; needs one repro | Triage |

## Queue (first pass)

| ID | Edge file / area | Edge change | Phoenix delta | Class | Decision | Smoke impact |
|----|------------------|-------------|---------------|-------|----------|--------------|
| EPQ-01 | `apu/audio_media_player.cc` | Destructor stops worker thread before fence teardown | Missing 3 lines | **A** | **ported** EDGE-PORT-A-1 | Menu/XMP shutdown; possible mission audio lifetime |
| EPQ-02 | `apu/audio_media_player.cc` | Linux: SDL driver for XMP (`SetupDriver`) | Missing `#if XE_PLATFORM_LINUX` block | **A** | **ported** EDGE-PORT-A-2 | Linux boot FMV / XMP |
| EPQ-03 | `gpu/d3d12/d3d12_shared_memory.cc` | Aborts `UploadRanges` when invalid range + cvar false | Phoenix warns + continues batch | **C** | skip | BF2 kortul; Phoenix policy is intentional (U-GPU-002) |
| EPQ-04 | `gpu/render_target_cache.cc` | No U-GPU-001 v2/v3 host-depth hooks | Phoenix has stale-tile resync + mismatch probe | **C** | skip | BF2 parked; do not regress |
| EPQ-05 | `apu/xaudio2/xaudio2_audio_driver.cc` | No `OnSubmitFramePcm` / `apu_trace` | Phoenix has Tier 1.4 telemetry hooks | **C** | skip | Keep Phoenix PCM hash logging |
| EPQ-06 | `kernel/xboxkrnl/xboxkrnl_audio.cc` | `XAudioQueryDriverPerformance` + simpler voice path | Phoenix has voice-category volumes + more exports | **B** | queue | APU smoke / G-454107DB-013 class issues |
| EPQ-07 | `apu/audio_system.cc` | `GetClientPerformance`, callback mutex, frame drop counters | Phoenix lacks performance API | **B** | queue | Pairs with EPQ-06 |
| EPQ-08 | `apu/xmp_state.h` + `kernel/xam/apps/xmp_app.*` | Typed `XmpClient` enum header | Phoenix uses inline `XMP_CLIENT` in app | **D** | refactor only | No functional delta found |
| EPQ-09 | `gpu/metal/*` (29 files) | Full Metal backend | Not in Phoenix | **B** | defer | macOS Phase 4.4a; not Windows smoke |
| EPQ-10 | `gpu/d3d12/d3d12_zpd_query_pool.*` | ZPD query pool | Missing in Phoenix | **D** | triage | Unknown title impact |
| EPQ-11 | `hid/keyboard/*` | Keyboard HID driver | Missing in Phoenix | **B** | queue | WoA / desktop QA |
| EPQ-12 | `kernel/util/shim_utils.h` | Shim differences | Phoenix has `LogKernelStubHitGuest` | **C** | skip | Tier 1.3 stub drain |
| EPQ-13 | `gpu/d3d12/pipeline_cache.cc` | Async / float24 PS plumbing | Both differ; BF2 kortul path | **B** | queue | BF2 revisit only |
| EPQ-14 | `gpu/command_processor.cc` | `readback_resolve` defaults / paths | Phoenix sets fast default | **C** | skip | G-454107DB-002 resolved |
| EPQ-15 | `base/main_win.cc` | Window / focus behavior | Diverged (Phoenix probe, flags) | **C** | skip | |
| EPQ-16 | `kernel/xam/xam_notify.cc` etc. | XAM UI / profile | Phoenix BF2 promotions landed | **C** | skip | BF2 stubs green |
| EPQ-17 | `apu/xma_decoder.cc` | Decoder paths | Both differ slightly | **D** | triage after XMA JSONL from smoke | |
| EPQ-18 | `apu/conversion.h` | NEON / conversion | Phoenix ARM64 notes in ledger | **B** | defer Phase 4 | |
| EPQ-19 | `cpu/ppc/ppc_emit_altivec.cc` | VMX emit | Phoenix 1M fuzz closed | **C** | skip | Do not perturb JIT |
| EPQ-20 | `kernel/xboxkrnl/xboxkrnl_video.cc` | Video / EDRAM | Overlaps Vd* BF2 work | **B** | queue | Boot video if regress on Phoenix |

## Queue (2026-05-23 re-audit — commits since fork)

| ID | Edge commit | Edge change | Phoenix delta | Class | Decision | Notes |
|----|-------------|-------------|---------------|-------|----------|-------|
| EPQ-21 | `e6ef86b9` / `5c82c5c` | Remove duplicate `Profiler::Dump()` | Missing 1 line | **A** | **ported** 2026-05-23 | `xenia_main.cc` |
| EPQ-22 | `d80e6e44` / `b0a1ea5` | `ClientSlot` default-init (no memset over mutex) | Partial — no EPQ-07 perf counters | **A** | **ported** 2026-05-23 | macOS/Linux audio robustness |
| EPQ-23 | `fcc4a22` / `59630a6` | NtFree/Protect heap null-check + devkit logging | Phoenix had heap deref without null | **A** | **ported** 2026-05-23 | `xboxkrnl_memory.cc` |
| EPQ-24 | `fb589f58` / `7a41c97` | GPU trace-writer guest addresses; drop `LoadShader` guest arg | Landed + `bc8e455ac` Vulkan PBE fix | **A** | **ported** 2026-05-23 | Tier 0 replay / trace capture |
| EPQ-25 | `cebbdb6` | `XeCryptHmacShaInit/Update/Final` | Already present | **A** | skip (empty pick) | |
| EPQ-26 | `6181160` | XEX swap prefix fix | Already present via canary sync | **A** | skip (empty pick) | |
| EPQ-27 | `3811fbf` | Posix relaunch dangling `argv` | Phoenix lacks posix relaunch block | **B** | queue | Land with Linux return-to-UI work |
| EPQ-28 | `250fb40` | `AsioErrorToWSAError` in `xsocket.cc` | Native-handle xsocket; no asio dep | **A** | **landed** 2026-05-23 | `2ac905f80` Phoenix-native errno→WSAE switch (no asio) — [edge_tier_ab_queue.md](edge_tier_ab_queue.md) |
| EPQ-29 | `0a82080` | Bake `gamecontrollerdb.txt` into exe | Missing `embed_bundle.py` infra | **B** | queue | Needs build-system port |
| EPQ-30 | `b975095` + `c2f7fc4` | JIT ITrace/DTrace/FTrace + x64 codegen fix | Pairs with Phoenix `xe::obs` | **A** | **landed** 2026-05-23 | Tier S S1 — `9f9b53a5e`, `0b800bae7` — [edge_tier_s_queue.md](edge_tier_s_queue.md) |
| EPQ-31 | `ba490f79` | `NetDll_WSAEventSelect` via asio | Phoenix native stub (no asio sockets) | **B** | **landed (partial)** 2026-05-23 | Tier B stub + `2ac905f80` poll-on-op signal; full async_wait Phase 2.5 |
| EPQ-32 | `45d770a7` + `5d4a90b3` + `1f4bb500` | HID keyboard merge + XInput removal + SDL JoystickType | Multi-commit unit | **B** | queue | WoA / desktop QA |
| EPQ-33 | `882cd7e9` | Game library storage separate from dash GPD | Overlaps launcher icon-cache WIP | **B** | queue | Human review vs stashed WIP |
| EPQ-34 | `4b1b62f7` | Startup profile creation + game import flow | UX feature | **B** | queue | Design review |
| EPQ-35 | `71dcd500` + `a1de6bd4` + `6e9c4973` | Volume / back-button / per-game FPS limit UI | Coherent trio | **B** | queue | UX bundle |
| EPQ-36 | `96effa1e` | ImGui GPU trace button | Tier A port to Display / settings panel | **A** | **landed** 2026-05-23 | Tier A — [edge_tier_ab_queue.md](edge_tier_ab_queue.md) |
| EPQ-37 | `b575c684` | XMA RexGlue / AC6_recomp derivation | Partially covered by canary `09dbe2c` | **D** | triage | Compare after smoke XMA JSONL |
| EPQ-38 | Metal + MoltenVK + macOS CI (~20 commits) | Full macOS Vulkan/Metal stack | EPQ-09 defer | **B** | defer | Phase 4.4a |

## Landed (class A)

| Tag | Commit / note | Files |
|-----|---------------|-------|
| EDGE-PORT-A-1 | 2026-05-17 | `audio_media_player.cc` destructor worker stop |
| EDGE-PORT-A-2 | 2026-05-17 | `audio_media_player.cc` Linux SDL XMP driver |
| EDGE-PORT-A-3 | 2026-05-23 | `xenia_main.cc` duplicate Profiler::Dump removal |
| EDGE-PORT-A-4 | 2026-05-23 | `audio_system.{cc,h}` ClientSlot default-init |
| EDGE-PORT-A-5 | 2026-05-23 | `xboxkrnl_memory.cc` NtFree/Protect null-check |
| EDGE-PORT-A-6 | 2026-05-23 | GPU trace-writer guest addresses (`command_processor`, D3D12/Vulkan/null) |
| EDGE-TIER-A | 2026-05-23 | 10 Edge picks: Vulkan diagnostics, base robustness, GPU trace UI — [edge_tier_ab_queue.md](edge_tier_ab_queue.md) |
| EDGE-TIER-B | 2026-05-23 | POSIX XThread/timer APC prep + NetDll `WSAEventSelect` (partial EPQ-28/31) — [edge_tier_ab_queue.md](edge_tier_ab_queue.md) |
| EDGE-TIER-S | 2026-05-23 | Tier S complete + follow-ups: S1 JIT trace, S3 xtimer, S4 XObject, S7 APU pair, S5 float24 PS (`731b19d86`), S6 readback barriers (partial) + `d1a4cf163` N/A — [edge_tier_s_queue.md](edge_tier_s_queue.md) |

## Held (watch / blocked)

| ID | Edge | Reason |
|----|------|--------|
| EPQ-S4-watch | `944e395c3` | Intrusive ZPD refactor; revisit after Tier S settles and FSI counter use is exercised by Tier 1.2 retail captures |

## Next B items (after smoke §2)

1. EPQ-06 + EPQ-07 together (audio performance + kernel query).
2. EPQ-20 if boot FMV regresses on Phoenix vs Edge baseline (G-454107DB-012).
3. EPQ-11 if keyboard needed on smoke platform.

## References

- [bf2_gpu_roadmap.md](bf2_gpu_roadmap.md) § Parked
- [30_debt_ledger.md](30_debt_ledger.md)
- [canary_resync_queue.md](canary_resync_queue.md)
- [master_review.md](master_review.md)
- [xenios_reference_notes.md](xenios_reference_notes.md)
