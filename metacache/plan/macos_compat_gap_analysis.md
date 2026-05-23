# macOS compat gap analysis (Phase 4.4a)

Last updated: **2026-05-16**. **Roadmap:** [arm64_compat_roadmap.md](arm64_compat_roadmap.md) Phase 4.4a. Native **Apple Silicon** (`XE_TARGET_AARCH64`).

Machine state: [macos_status.md](../../xenia-phoenix-src/docs/macos_status.md).

## XeniOS reference (local + [GitHub](https://github.com/xenios-jp/XeniOS))

**Local path:** `g:\Xenia-Xenia Canary\XeniOS\` (workspace sibling of `Xenia-Phoenix/`). Branch **`xenios`**. Read-only — no submodule inside Phoenix.

Apple-focused fork with **native Metal GPU** ([`XeniOS/src/xenia/gpu/metal/`](../../../XeniOS/src/xenia/gpu/metal/)), **Qt 6 UI** (`window_qt.cc`), iOS app, premake/`xb` build. **Does not include** Phoenix Tier 0/1 tools.

| Use local XeniOS for | Do not port |
|----------------------|-------------|
| Diff `surface_mac.mm`, `gpu/vulkan`, `kernel`, `cpu/backend/a64` | `gpu/metal/`, `window_qt*`, premake, iOS |
| Packaging: `xenia.entitlements`, `assets/` | Bulk merge or submodule |
| Pre-PR audit when touching Mac GPU/UI | Qt UI stack |

Phoenix strategy: **Cocoa + MoltenVK (Vulkan)**. Tier 1 on Mac = Phoenix **measurement scripts** + `xenia_main` Vulkan factory fix.

## xenia-edge reference ([has207/xenia-edge](https://github.com/has207/xenia-edge))

Canary-lineage fork (CMake + `xenia-build.py`). **Parent of XeniOS** per XeniOS README. macOS ships **Metal + wxWidgets**, not MoltenVK—same `!APPLE` Vulkan exclusion class as Phoenix’s factory bug, but Edge *intentionally* uses `gpu/metal` on Mac.

| Use Edge for | Do not port |
|--------------|-------------|
| CMake APPLE block, CI `.app`/DMG patterns, `xenia-build.py` darwin | `gpu/metal/`, `ui/wx*`, Metal Toolchain CI steps |
| Intel Mac cross-build (`--target-arch=x64`) when Phoenix expands matrix | wxWidgets, game-patches `fetchdata` coupling |

**Pre–bare-metal:** Edge is the **primary infra reference**; XeniOS is secondary (MoltenVK/Cocoa file diffs only). Optional local clone: `../../xenia-edge` branch `edge`. See [macOS Pre-Bare-Metal plan](file:///c:/Users/khutt/.cursor/plans/macos_pre-bare-metal_7eecac05.plan.md).

## Summary

| Area | Status |
|------|--------|
| CPU / a64 JIT | High — same backend as Windows ARM64 |
| UI (Cocoa) | Landed — `window_mac.mm`, `windowed_app_main_mac.mm` |
| Vulkan / MoltenVK | Landed — `VK_EXT_metal_surface`, loader via `VULKAN_SDK` |
| APU / HID | SDL2 system framework |
| CI | `build-macos.yml` on `macos-15` |
| Signing / notarization | Out of scope pre-push |

## Build matrix

| Host | Arch | Backend | CI |
|------|------|---------|-----|
| macOS 15+ | arm64 native | a64 | `build-macos.yml` |
| macOS x64 (Intel) | x64 | x64 | Not targeted in CI |

## Subsystems

| Subsystem | Notes |
|-----------|-------|
| CMake | `VK_USE_PLATFORM_METAL_EXT`; Vulkan + glslang-spirv enabled on APPLE |
| UI | Cocoa + `CAMetalLayer`; stub `MacMenuItem` |
| GPU | Full `gpu/vulkan` + MoltenVK |
| Tests | `xenia-cpu-tests` via `macos-verify.sh` |

## Known gap (runtime)

- [ ] [`xenia_main.cc`](../../xenia-phoenix-src/src/xenia/app/xenia_main.cc) — `#if !XE_PLATFORM_MAC` still excludes `VulkanGraphicsSystem` factory while CMake links vulkan

## Tier 1 on macOS (Phoenix-specific)

| Gate | Mac status |
|------|------------|
| vmx128-fuzz 1M | Tool portable; run on a64 — same blockers as Windows x64 |
| gpu_replay_ci | Needs `bin/macOS` + vulkan backend in `run.py` (no D3D12 RTV/ROV) |
| xma2-diff / stub aggregate | Python — portable |
| smoke capture | Needs `run_smoke_capture.sh` (PS1 is Windows-only) |
| patch debt | Portable |

## Manual gates

- [ ] [macos_runtime_checklist.md](../../xenia-phoenix-src/docs/macos_runtime_checklist.md)
- [ ] [60_smoke_titles.md](60_smoke_titles.md) macOS ARM64 column
