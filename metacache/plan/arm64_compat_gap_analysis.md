# ARM64 compat gap analysis (Phase 4.0)

Last updated: **2026-05-16**. **Roadmap:** Phase **4.x** in [20_tier_roadmap.md](20_tier_roadmap.md) (not Phase 2 Linux x86_64, not Phase 3 Android). **Policy:** Windows ARM64 (WoA) first; Linux aarch64 and macOS deferred.

Machine state: [arm64_status.md](../../xenia-phoenix-src/docs/arm64_status.md).

## Feasibility summary

| Dimension | Rating | Notes |
|-----------|--------|-------|
| CPU / JIT | High | Full `cpu/backend/a64/` (12 `.cc`), xbyak_aarch64, mirrors x64 tree |
| Platform | High | `XE_ARCH_ARM64` in `platform.h`, `platform_arm64.*`, exceptions, stack walk |
| GPU (D3D12) | Medium–High | NEON in `primitive_processor`; D3D12 scalar fallbacks when not AMD64 |
| APU | Medium | SSE paths x64-only; scalar on ARM64 (correct, slower) |
| Build / CI | Low → in progress | `xenia-build.py --target-arch arm64`; CI workflow added |
| Linux aarch64 | Deferred | After WoA; requires Phase 2.5+ Linux x86_64 runtime sign-off |

## Build matrix

| Target | Host | Tooling | Status |
|--------|------|---------|--------|
| Windows ARM64 native | WoA / ARM64 VM | MSVC ARM64, `build/` or `build-arm64/` | Supported via `xenia-build.py` |
| Windows ARM64 cross | x64 + VS | `VC.Tools.ARM64`, `build-arm64/` | Supported via `xenia-build.py` |
| Linux aarch64 | aarch64 host | clang, native CMake | CMake OK; no Phoenix CI/scripts yet |
| macOS Apple Silicon | M-series | Xcode | CMake `APPLE`; no CI |

## Subsystem inventory

### CPU

| Item | x64 | a64 |
|------|-----|-----|
| Backend tree | `cpu/backend/x64/` | `cpu/backend/a64/` |
| JIT lib | xbyak | xbyak_aarch64 |
| CMake | `XE_TARGET_X86_64` | `XE_TARGET_AARCH64` |
| Runtime | `cpu=x64` / `any` | `cpu=a64` / `any` |
| Known gap | — | Saturation tracking TODO `a64_sequences.cc`; `UnimplementedInstr` for unhandled HIR |

### Base / platform

| File | ARM64 |
|------|-------|
| `base/platform_arm64.{h,cc}` | Feature flags (LSE, FPCR) |
| `base/clock_arm64.cc` | Clock |
| `base/memory.cc` | memcpy path (no MOVDIR64B) |
| `base/exception_handler*` | ARM64 handlers |

### GPU

| Path | ARM64 |
|------|-------|
| `gpu/primitive_processor.*` | `arm_neon.h` path |
| `gpu/d3d12/*` | Scalar `#else` when not `XE_ARCH_AMD64` |

### APU

| Path | ARM64 |
|------|-------|
| `apu/conversion.h`, `xma_context.cc` | Scalar only (`XE_ARCH_AMD64` SIMD) |

### Third_party (CMake)

| Dep | AArch64 |
|-----|---------|
| Capstone | AArch64 arch |
| FFmpeg | `libavutil/aarch64`, `libavcodec/aarch64` |
| zlib-ng | arm/neon paths |
| discord-rpc RapidJSON | `RAPIDJSON_NEON` via `XE_TARGET_AARCH64` (was typo `XE_TARGET_ARM64`) |

### Tests / tools

| Target | ARM64 |
|--------|-------|
| `xenia-cpu-tests` | Links `xenia-cpu-backend-a64` |
| `vmx128-fuzz` | Scalar ref on ARM64; SSE families x64-only |
| GPU trace dump tools | x64 backend only in CMake today |

## Known bugs (fixed or tracked)

| Bug | Fix |
|-----|-----|
| MSVC `_AMD64=1` on all Windows builds | Gated behind `XE_TARGET_X86_64` |
| `XE_TARGET_ARM64` in discord-rpc CMake | Renamed to `XE_TARGET_AARCH64` |
| Missing `build-win_arm64.yml` | Added; Orchestrator job enabled |
| Orchestrator ARM64 job commented | Uncommented |

## Automated vs manual gates

| Gate | Type | Status |
|------|------|--------|
| ARM64 Release compile (CI) | Automated | `build-win_arm64.yml` |
| `arm64-verify.ps1` smoke + cpu-tests | Automated (WoA emulation on x64 runner where possible) | Script added |
| vmx128-fuzz on ARM64 | Manual / ARM64 hardware | Deferred until WoA runner or device |
| Runtime checklist (UI, D3D12, audio, HID) | Manual | Pending — see `arm64_windows_build.md` |
| [60_smoke_titles.md](60_smoke_titles.md) WoA column | Manual | Template added |

## Phase 4 sub-phase mapping

| Sub-phase | Goal |
|-----------|------|
| **4.0** | This gap analysis |
| **4.1** | Windows ARM64 CI compile green |
| **4.2** | `xenia-cpu-tests` (+ fuzz on hardware when available) |
| **4.3** | Runtime smoke on WoA |
| **4.4** | Linux aarch64 CI (deferred) |

## macOS Apple Silicon (Phase 4.4a)

See [macos_compat_gap_analysis.md](macos_compat_gap_analysis.md). Native arm64 uses the same `a64` backend; presentation via MoltenVK (`VK_EXT_metal_surface`).

## Linux aarch64 (Phase 4.4b)

CI: `build-linux_arm64.yml` on `ubuntu-24.04-arm`. Runtime gates follow Phase 2 Linux x86_64 sign-off.

## Distinction from Phase 3 Android

Android uses `a64` backend inside `libxenia-app.so` but is a separate product (JNI, Gradle, scoped storage). Do not merge Android and desktop ARM64 checklists.
