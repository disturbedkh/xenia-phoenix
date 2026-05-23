# Canary resync queue

**Fork point:** `c2674b19d2` (2026-05-15) — `[Vulkan] Drop vertex buffer residency cache, hoist global lock like D3D12`

**Phoenix sync branch:** `sync/canary-2026-05-21` → merged into `canary_experimental` at `c386deb44` (2026-05-23)

**Upstream tip synced:** `09dbe2cd3` (2026-05-21) — `[APU] XmaContextNew: do not invalidate output_buffer_valid prematurely`

**Method:** Individual cherry-picks from `upstream/canary_experimental` onto Phoenix `2e62ccabe`, with build gates between batches.

## Pick chronology

| # | Upstream | Date | Subject | Conflicts | Phoenix commit | Build gate |
|---|----------|------|---------|-----------|----------------|------------|
| 1 | `dc4db67` | 2026-04-11 | `[Kernel] Change OddObj to X_DISPATCH_HEADER` | Auto-merge (kernel_state, memory, threading) | `fb40c32ac` | Batch 1 — green after full rebuild |
| 2 | `a261b83` | 2026-05-14 | `[APP] Flush log & save config on quick exit` | Auto-merge | `13a109ab5` | Batch 1 |
| 3 | `4c396fe` | 2026-05-16 | `[XAM] LanguageLocaleFallback + Typeface` | Auto-merge | `54609e128` | Batch 2 |
| 4 | `482629a` | 2026-05-17 | `[XAM] FormatDate/Time param count` | Clean | `132147e47` | Batch 2 |
| 5 | `99ea6da` | 2026-05-17 | `[XConfig] Implementation of XConfig` | **Manual:** `emulator_window.{cc,h}`, `kernel_flags.cc`, `xboxkrnl_xconfig.cc` — kept Phoenix launcher dialogs + `kernel_stub_hit_log`; took canary `xconfig.{cc,h}` + `ConsoleSettingsDialog` | `bb343be3b` | Batch 2 |
| 6 | `ef67d1c` | 2026-05-18 | `[SMC] std::bind → lambda` | Clean | `4c615be1b` | Batch 3 |
| 7 | `f88bfbe` | 2026-05-18 | `[Xam/Content] XamContentCreateEnumeratorInternal` | Clean | `8f9e963e0` | Batch 3 |
| 8 | `9ce91e5` | 2026-05-19 | `[UI] Listed extended languages` | Clean | `49d3aa938` | Batch 3 |
| 9 | `072ef7e` | 2026-05-19 | `[XAM] XamGetOnlineLanguageAndCountry` | Clean | `f278c6731` | Batch 3 |
| 10 | `a11908f` | 2026-05-19 | `[XAM] Enable storing IPTV name` | Clean | `b4756dd16` | Batch 3 |
| 11 | `b2aa8b2` | 2026-05-20 | `[BASE] Use FlushAllSinks instead of inline loop` | Auto-merge `logging.cc` | `080093e75` | Batch 4 |
| 12 | `09dbe2c` | 2026-05-21 | `[APU] XmaContextNew output_buffer_valid` | Auto-merge — **Phoenix `apu_trace` / `LogXmaDivergence` preserved** | `cc5a26d7f` | Batch 4 |

## Phoenix-only follow-ups (same session)

| Commit | Reason |
|--------|--------|
| `c386deb44` | Restore `internal_display_resolution{,_x,_y}` cvars in `graphics_system.cc` after XConfig merge removed them — fixes `graphics_settings_dialog` link |
| `bc8e455ac` | Drop Vulkan `OnPrimaryBufferEnd` override (Edge trace-writer pick added declaration without Edge's Vulkan PBE implementation) |

## Build / test gates

| Gate | Result | Notes |
|------|--------|-------|
| Release `xenia-app` | **Green** | Clean `build/` required (CMake cache pointed at old `G:\Xenia-Xenia Canary\...` path) |
| `xenia-cpu-tests` | **Green** | 800 assertions / 250 cases |
| `vmx128-fuzz` 5k sample | **Sparse mismatches + abnormal exit** | Known low-iter flake on `vaddfp128`/`vmaxfp`/`vrfip*`; not a canary-regression signal vs closed 1M gate |
| `vmx128-fuzz` 50k | **Crash at startup** | Pre-existing harness issue when run ad-hoc; CI uses 50k in controlled workflow |

## Conflict playbook (for next resync)

- **XConfig (`99ea6da`):** Always merge Phoenix launcher (`ShowLauncher`, `GraphicsSettingsDialog`, `LibrarySettingsDialog`) alongside canary `ConsoleSettingsDialog` + `xconfig` class.
- **XmaContextNew (`09dbe2c`):** Never drop `apu_trace.h` hooks or `--apu_xma_divergence_log` paths.
- **kernel_flags:** Keep Phoenix `kernel_stub_hit_log` cvar when taking canary category renames.

## References

- [edge_port_queue.md](edge_port_queue.md)
- [20_tier_roadmap.md](20_tier_roadmap.md)
- Session: [cache/session_log.md](../cache/session_log.md) § 2026-05-23 Canary catch-up
