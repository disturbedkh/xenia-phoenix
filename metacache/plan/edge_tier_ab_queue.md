# Edge Tier A+B port queue

**Purpose:** Cherry-pick 14 Edge commits into Phoenix across two scoped branches (Tier A diagnostic/enabling stack, Tier B NetDll + POSIX kernel prep). A4 ZPD refactor held.

**Branches:** `sync/edge-tier-a-2026-05-23` → FF `canary_experimental`; `sync/edge-tier-b-2026-05-23` → FF `canary_experimental`.

**Edge audit tip:** `6e02a106c` (2026-05-23). **Phoenix HEAD after merge:** `74fe77ce0` on `canary_experimental` (2026-05-23; Tier S landed).

**Pre-flight base:** `828b7c40a` (not plan's `bc8e455ac` — canary doc pointer was stale).

**Post Tier A+B net fixup:** `2ac905f80` — Phoenix-native errno→WSAE + `MaybeSignalSelectedEvent` (EPQ-28/31 functional; no asio).

---

## Branch A — `sync/edge-tier-a-2026-05-23` (10 Edge picks + 2 Phoenix)

| # | Edge | Subject | Conflicts | Phoenix commit | Notes |
|---|------|---------|-----------|----------------|-------|
| A3-1 | `435bea911` | Vulkan swapchain failure logging | Clean | `e81ad7079` | |
| A3-2 | `883109527` | SIGBUS handler restore | **Manual** | `3e30328df` | Added `original_sigbus_handler_` + restore in `exception_handler_posix.cc` |
| A3-3 | `9f8c68b9f` | Linux uncaught exception log | **Manual** | `99f27b649` | Includes: `xenia/base/logging.h`, `cxxabi.h` |
| A3-4 | `4839365b2` | `quick_exit` on FatalError | Clean | `07d039b9c` | Profiler HTML dump still runs via existing atexit path |
| A3-5 | `50a630c03` | Cvar override leak fix | **Manual** | `76731fd97` | Kept Phoenix `OVERRIDE`/`OVERRIDE_PERSIST` in `cvar.h`; dropped Edge-only `Update*Cvar`; removed deleted `imgui_debug_dialog.cc`; restored graphics validation block + 1280×720 defaults |
| A2-1 | `6f410ebfc` | `vulkan_validation` multi-level | Clean | `200fd637c` | |
| A2-2 | `7be6ecf54` | 16-slot submission summary | **Major** | `4421b4e4f` | Manual port: `SubmissionSummary`, `LogRecentSubmissions`, hooks in IssueDraw/IssueCopy/BeginSubmission/EndSubmission |
| A2-3 | `991997046` | Device-loss submission context | **Manual** | `c95ef46ed` | Merged enhanced logging + kept `LogRecentSubmissions` |
| A2-4 | `bf2f984f7` | VK_EXT_device_fault | **Manual** | `663225709` | `features_EXT_device_fault` only (not Edge's extra feature structs) |
| A1 | `96effa1eb` | ImGui GPU trace button | **Phoenix port** | `ba4616de2` | No `imgui_debug_dialog.cc` — button in Display dialog (`emulator_window.cc`); moved to `display_settings_panel.cc` when settings UI WIP restored |
| — | — | Phoenix build fixup | — | `b46ee11f9` | `internal_display_resolution_entries` → 1280×720; `ucode_data_hash()` fix |

### Branch A gate

| Gate | Result |
|------|--------|
| Release `xenia-app` + `xenia-cpu-tests` | **Green** |
| `xenia-cpu-tests` | 800 assertions / 250 cases |
| 5-min smoke `--vulkan_validation=2` | Run in session (validation levels exercised) |

---

## Branch B — `sync/edge-tier-b-2026-05-23` (4 Edge picks + 1 Phoenix)

| # | Edge | Subject | Conflicts | Phoenix commit | Notes |
|---|------|---------|-----------|----------------|-------|
| B2a | `ace49597b` | POSIX XThread release cleanup | Clean | `f23bc62c4` | `#if LINUX/ANDROID/MAC` — no-op Windows |
| B2b | `0672a6872` | Timer APC without guest TLS | Clean | `55d147623` | Defensive `EnqueueApc` — harmless Windows |
| B1a | `250fb40dc` | `AsioErrorToWSAError` (EPQ-28) | **Manual** | `03eaec9b2` | Phoenix has no `asio` in third_party; pick landed structurally, function removed in fixup |
| B1b | `ba490f79e` | `NetDll_WSAEventSelect` | **Manual** | `57b56c07c` | `xam_net.cc` clean; native-handle `WSAEventSelect` stub (non-blocking `FIONBIO`/`fcntl`) |
| — | — | Phoenix net fixup | — | `02f2c540c` | Dropped `asio.hpp`; EPQ-28 WSAE map **deferred** until asio socket migration (Phase 2.5 networking) |
| — | — | Phoenix net fixup (follow-up) | — | `2ac905f80` | Full errno→WSAE switch table + `MaybeSignalSelectedEvent` poll-on-op (EPQ-28 **landed**, EPQ-31 partial) |

### Branch B gate

| Gate | Result |
|------|--------|
| Release `xenia-app` + `xenia-cpu-tests` | **Green** |
| `xenia-cpu-tests` | 800 assertions / 250 cases |
| POSIX picks (Windows) | Compile-only sanity — no runtime behavior change |

---

## Held / out of scope

| Item | Edge | Reason |
|------|------|--------|
| A4 | `944e395c3` ZPD refactor | EPQ-S4-watch |
| Tier S | JIT trace, kernel strings, KeFP, XObject, Vulkan depth trio, APU pair | **Partial** — [edge_tier_s_queue.md](edge_tier_s_queue.md) @ `74fe77ce0` |
| Tier C/D/F | UI bundles, macOS/Metal, AC6 ground-fix, fake ZPD | Deferred |

---

## Stash / WIP

| Stash | Note |
|-------|------|
| `wip-settings-ui-pre-tier-s` | Settings UI refactor. Stashed for Tier S branch; restored post-merge (mechanical). |

---

## References

- [edge_port_queue.md](edge_port_queue.md)
- [20_tier_roadmap.md](20_tier_roadmap.md)
- [canary_resync_queue.md](canary_resync_queue.md)
- Session: [cache/session_log.md](../cache/session_log.md) § 2026-05-23 Edge Tier A+B
