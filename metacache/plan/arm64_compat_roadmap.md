# ARM64 compatibility roadmap (Phoenix)

Numbering: **Phase 4** = desktop/host ARM64 (global phase in [20_tier_roadmap.md](20_tier_roadmap.md)). Not Phase 2 Linux x86_64, not Phase 3 Android.

Machine state: [arm64_status.md](../../xenia-phoenix-src/docs/arm64_status.md). Build guide: [arm64_windows_build.md](../../xenia-phoenix-src/docs/arm64_windows_build.md).

**Policy:** Windows ARM64 (WoA) first; Linux aarch64 and macOS after Phase 4.3 exit.

## Phase 4.0 — Gap analysis

- [x] [arm64_compat_gap_analysis.md](arm64_compat_gap_analysis.md) (2026-05-16)

## Phase 4.1 — Windows ARM64 build + CI

- [x] CMake: gate `_AMD64=1` to x86_64 only; fix `XE_TARGET_AARCH64` for discord-rpc NEON
- [x] `xenia-build.py` fail-fast when ARM64 MSVC tools missing
- [x] [arm64_windows_build.md](../../xenia-phoenix-src/docs/arm64_windows_build.md)
- [x] [build-win_arm64.yml](../../xenia-phoenix-src/.github/workflows/build-win_arm64.yml) + Orchestrator job
- [x] [arm64-verify.ps1](../../xenia-phoenix-src/tools/tier0/arm64-verify.ps1)
- [ ] First green CI run on `windows-2025` (requires runner with ARM64 build tools)

## Phase 4.2 — CPU validation

- [ ] `xenia-cpu-tests` green on ARM64 build (CI or WoA hardware)
- [ ] `vmx128-fuzz` on ARM64 hardware (`cpu=a64`); document in [arm64_status.md](../../xenia-phoenix-src/docs/arm64_status.md)
- [ ] Saturation tracking TODO in `a64_sequences.cc` if fuzz exposes divergences
- [x] Audit: `UnimplementedInstr` is fallback trap (same as x64); no extra call sites in a64 tree — runtime hits indicate missing HIR lowering

## Phase 4.3 — Runtime smoke (manual)

- [ ] WoA checklist: window, ImGui, D3D12, audio, HID — see [arm64_windows_build.md](../../xenia-phoenix-src/docs/arm64_windows_build.md)
- [ ] [60_smoke_titles.md](60_smoke_titles.md) **Windows ARM64** column — ≥1 green title

## Phase 4.4a — macOS Apple Silicon

- [x] [macos_compat_gap_analysis.md](macos_compat_gap_analysis.md)
- [x] Cocoa UI + `VK_EXT_metal_surface` / MoltenVK
- [x] [build-macos.yml](../../xenia-phoenix-src/.github/workflows/build-macos.yml) + Orchestrator
- [x] [macos-verify.sh](../../xenia-phoenix-src/tools/tier0/macos-verify.sh), docs
- [ ] First green macOS CI run
- [ ] [macos_runtime_checklist.md](../../xenia-phoenix-src/docs/macos_runtime_checklist.md) manual

## Phase 4.4b — Linux aarch64

- [x] [build-linux_arm64.yml](../../xenia-phoenix-src/.github/workflows/build-linux_arm64.yml) on `ubuntu-24.04-arm`
- [ ] First green Linux ARM64 CI run
- [ ] Runtime parity (Phase 2.5+ on x86_64 Linux first)

Do not block 4.1–4.3 on 4.4.

## Automated gates (target)

Every PR: Windows x64 (existing) + **Windows ARM64 compile** + `arm64-verify` smoke/cpu-tests when emulation or ARM64 runner permits.

Manual gates: Phase 4.3 runtime + smoke titles. Tier 1 gameplay accuracy stays **Windows x64-first** until ARM64 smoke column has ≥1 green title.
