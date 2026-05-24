# Edge Tier S port queue

**Purpose:** Cherry-pick 19 Edge commits (7 clusters) into Phoenix on `sync/edge-tier-s-2026-05-23`, follow-ups on `sync/edge-tier-s-followups-2026-05-23`, gate, FF merge to `canary_experimental`.

**Branch:** Tier S partial `sync/edge-tier-s-2026-05-23` from `2ac905f80` → FF `canary_experimental` @ `74fe77ce0`. Follow-ups `sync/edge-tier-s-followups-2026-05-23` from `74fe77ce0` → FF `canary_experimental`. **Phoenix HEAD after follow-ups:** `466804473` on `canary_experimental` (2026-05-23).

**Edge audit tip:** `6e02a106c` (2026-05-23).

---

## Pick chronology (19 Edge commits)

| Cluster | Edge | Subject | Conflicts | Phoenix commit | Notes |
|---------|------|---------|-----------|----------------|-------|
| S1 | `b975095fd` | JIT ITrace/DTrace/FTrace | **Manual** | `9f9b53a5e` | Kept Phoenix `xe::obs`; CMake + `xenia-build.py` + deleted `imgui_debug_dialog.cc` resolved |
| S1 | `c2f7fc48c` | x64 latent JIT data-tracing codegen bugs | **Manual** | `0b800bae7` | Merged with Phoenix x64 emitter / pin table |
| S6 | `63dbbf094` | drop kEarlyHint in GetCurrentPixelShaderModification | **Manual** | `5d0616618` | Phoenix-native: remove kEarlyHint only (no float24 enum — needs S5/731) |
| S6 | `29b03f9ea` | barrier readback buffer writes | **Manual** | `e6e54bf8a` | 2/3 sites on Phoenix (memexport sync path + resolve readback); scaled-resolve path **N/A** (no `resolve_downscale_buffer_`) |
| S6 | `631d7d2b5` | Drop stale FenceAcquisition operator== | Clean | `3d439f40d` | |
| S6 | `d1a4cf163` | dynamic-rendering attachment slot count | **N/A** | — | Phoenix uses `RenderPassKey` + traditional `vkRenderPass`; VK_KHR_dynamic_rendering not adopted |
| S6 | `3087c4c5d` | Barrier consecutive uploads same image | Clean | `74fe77ce0` | Includes kEarlyHint Phoenix-native fix in same amend |
| S6 | `562aa0dd9` | clip/cull distance array declare-one | Empty | — | Already in Phoenix |
| S6 | `02a259129` | ac6_ground_fix SPIR-V fetch nudge | Empty | — | `spirv_shader_translator_fetch.cc` already has `ac6_ground_fix` |
| S5 | `731b19d86` | depth_float24_convert_in_pixel_shader | **Manual** | `e6e54bf8a`, `466804473` | Phoenix-native SPIR-V enum + RTC accessor + pipeline cache; `draw_util::GetNormalizedDepthControl(regs)` in pipeline cache (not arg list) |
| S5 | `b15fcc73e` | oDepth via gl_FragDepth + FSI | Empty | — | `output_or_var_fragment_depth_` + FSI path already in Phoenix spirv |
| S5 | `cb5ad9a11` | FSI_DepthStencilTest consumes shader depth | Empty | — | `FSI_DepthStencilTest` oDepth branch already in Phoenix |
| S2 | `fdbaaaba2` | kernel string exports (not SHIM) | Empty | — | Already in Phoenix |
| S3 | `331ddf79b` | KeGetImagePageTableEntry bit 0 | Empty | — | Already in Phoenix |
| S3 | `cdb88d0b3` | DPC Remove IsQueued check | Empty | — | Already in Phoenix |
| S3 | `353a7a00a` | xtimer logspam → XELOGD | Clean | `dcce43dc0` | |
| S7 | `8aa50e0e0` | APU per-client callback mutex | Clean | `28f204871` | Preserved `apu_trace` hooks |
| S7 | `c26d93763` | APU tick semaphore submit-after-unregister | Clean | `e86c78bcd` | |
| S4 | `3341c7aa8` | XObject 32-byte alignment | Clean | `be9dff97d` | Capstone pick |

**Pre-Tier-S net fix (not on Tier S branch base):** `2ac905f80` — Phoenix-native errno→WSAE + `MaybeSignalSelectedEvent` (EPQ-28/31 functional).

---

## Tier S follow-ups gate (2026-05-23)

| Gate | Result | Notes |
|------|--------|-------|
| Release `xenia-app` + `xenia-cpu-tests` + `xenia-gpu-d3d12-trace-dump` | **Green** | Post-merge @ `466804473` |
| `xenia-cpu-tests` | **800 / 250** | |
| vmx128-fuzz 50k per-opcode (`vadd*`, `vupk*`, seed `3735928559`) | **Green** | Full registry still **AV** pre-existing — see triage below |
| GPU format-validate | **Green (exit 0)** | 11 traces |
| GPU strict (default cvars) | **Green (exit 0)** | Zero hash drift on synthetic corpus |
| Stub-trace 30s boot | **Not run** | No local roster `default.xex` path configured |

---

## Tier S gate (partial, 2026-05-23)

| Gate | Result | Notes |
|------|--------|-------|
| Release `xenia-app` + `xenia-cpu-tests` | **Green** | Post-merge @ `74fe77ce0` |
| `xenia-cpu-tests` | **800 / 250** | |
| vmx128-fuzz 50k (seed `3735928559`, `rm=rn`) | **Pre-existing harness crash** | Full registry AV on `canary_experimental` baseline too; per-opcode 50k OK; CI runs from `build/bin/Windows/Release` — see [canary_resync_queue.md](canary_resync_queue.md) |
| GPU format-validate | **Green (exit 0)** | 11 traces; pre-existing truncated kEvent tail warnings on slot_j |
| Stub-trace 30s boot | **Not run** | S2 empty skip |

---

## Conflict playbook (next Tier S-class op)

1. **Never `--theirs` whole `vulkan_command_processor.cc` / `vulkan_render_target_cache.cc`** — Edge ZPD + dynamic rendering + debug markers diverge from Phoenix Tier A SubmissionSummary stack.
2. **S5 float24 PS** — fetch `normalized_depth_control` via `draw_util::GetNormalizedDepthControl(regs)` inside `GetCurrentPixelShaderModification`; do not extend the caller signature unless all call sites need it.
3. **S5 SPIR-V compat** — `ExecutionModeDepthLess` and `DecorationSample` need aliases in `spirv_compatibility.h` for glslang spirv.hpp11 migration.
4. **S6 readback barriers** — Phoenix has sync-stall memexport (not Edge async fast path); scaled-resolve downscale path absent — only port sites that exist.
5. **S1 JIT trace** — keep Phoenix `xe::obs` + `vmx128_pin_table`; vmx128 gate is first bisect target if new divergences appear.
6. **WIP settings UI** — mechanical stash/pop around branch ops only (`wip-settings-ui-pre-tier-s-followups`).

---

## Held / follow-up

| Item | Edge | Reason |
|------|------|--------|
| vmx128 50k ad-hoc full registry | — | Pre-existing AV before any opcode output; `vadd*`/`vupk*` 50k green — see [cache/vmx128_runs.md](../cache/vmx128_runs.md) § Tier S follow-ups triage |
| S6 scaled-resolve barrier | `29b03f9ea` site 3 | **N/A** on Phoenix — no scaled-resolve compute readback path |
| EPQ-S4-watch | `944e395c3` | Intrusive ZPD refactor — not blocked by `d1a4cf163` N/A on Phoenix render-pass path |

---

## References

- [edge_tier_ab_queue.md](edge_tier_ab_queue.md)
- [edge_port_queue.md](edge_port_queue.md)
- [20_tier_roadmap.md](20_tier_roadmap.md)
- Session: [cache/session_log.md](../cache/session_log.md) § 2026-05-23 Tier S follow-ups
