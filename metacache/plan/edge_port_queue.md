# Edge port queue

**Purpose:** Harvest low-risk fixes from [Xenia-Edge](../../Xenia-Edge/) into `xenia-phoenix-src` without disturbing Phoenix Tier 1-PC wins (vmx128, U-GPU instrumentation, `apu_trace`, stub logging, upload continue).

**Audit method:** File hash compare + targeted `git diff --no-index` on differing files (2026-05-17). Edge HEAD `6101f13`; Phoenix HEAD `92869d77`. Fence-post commit `707052a2f` from compat #9 is not in local Edge clone.

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

## Landed (class A)

| Tag | Commit / note | Files |
|-----|---------------|-------|
| EDGE-PORT-A-1 | 2026-05-17 | `audio_media_player.cc` destructor worker stop |
| EDGE-PORT-A-2 | 2026-05-17 | `audio_media_player.cc` Linux SDL XMP driver |

## Next B items (after smoke §2)

1. EPQ-06 + EPQ-07 together (audio performance + kernel query).
2. EPQ-20 if boot FMV regresses on Phoenix vs Edge baseline (G-454107DB-012).
3. EPQ-11 if keyboard needed on smoke platform.

## References

- [bf2_gpu_roadmap.md](bf2_gpu_roadmap.md) § Parked
- [30_debt_ledger.md](30_debt_ledger.md)
- [workspace/00_monorepo_pointers.md](../workspace/00_monorepo_pointers.md)
