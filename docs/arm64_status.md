# Windows ARM64 port status

Last updated: **2026-05-23** (Phase 4.1 + 4.4b CI green on [Orchestrator 26341449305](https://github.com/disturbedkh/xenia-phoenix/actions/runs/26341449305)).

## Automated gates (CI) — Windows ARM64

- [x] `build-win_arm64.yml` — cross-compile via `xenia-build.py --target-arch arm64`
- [x] Orchestrator `build-windows-arm64` job enabled
- [x] `tools/tier0/arm64-verify.ps1` — PE machine check (0xAA64); execution smoke on ARM64 host only
- [x] First green CI run confirmed — [26335584966](https://github.com/disturbedkh/xenia-phoenix/actions/runs/26335584966); reconfirmed [26341449305](https://github.com/disturbedkh/xenia-phoenix/actions/runs/26341449305) with B36 Vulkan cache hardening

## Automated gates (CI) — Linux aarch64 (Phase 4.4b)

- [x] `build-linux_arm64.yml` on `ubuntu-24.04-arm` — LLVM 20 toolchain pinning, Vulkan SDK, doctor, smoke, cpu-tests (parity with `Linux_x86.yml`)
- [x] First green Linux ARM64 CI run — [26341449305](https://github.com/disturbedkh/xenia-phoenix/actions/runs/26341449305) (B28–B35)
- [ ] Runtime parity deferred until Phase 2.5+ Linux x86_64 sign-off

## Local dev (this machine)

- [ ] ARM64 MSVC component installed (`Microsoft.VisualStudio.Component.VC.Tools.ARM64`)
- `xenia-build.py setup --target-arch arm64` **fails fast** if cross-compiler missing (fixed 2026-05-16)

Install ARM64 tools: Visual Studio Installer → modify → Individual components → **MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools**.

## Manual gates (Phase 4.3)

- [ ] Window + ImGui on WoA
- [ ] D3D12 title boots
- [ ] Audio + input
- [ ] [60_smoke_titles.md](../../metacache/plan/60_smoke_titles.md) WoA column

## Subsystem status

| Subsystem | Build | Runtime verified |
|-----------|-------|------------------|
| CMake / a64 backend | CI + docs | — |
| CPU tests | CI (when tools + emulation OK) | — |
| D3D12 + NEON primitive path | Compiles | Manual |
| APU (scalar) | Compiles | Manual |
| UI (Win ARM64 TEB) | Compiles | Manual |

## Quick commands

```powershell
python xenia-build.py setup --target-arch arm64
python xenia-build.py build --target-arch arm64 --config=release --target=xenia-app --build-tests
.\tools\tier0\arm64-verify.ps1
```

See [arm64_windows_build.md](arm64_windows_build.md).
