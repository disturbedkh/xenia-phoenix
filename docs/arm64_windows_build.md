# Windows ARM64 build (Phoenix)

Host **AArch64** builds use the `a64` CPU JIT backend (`cpu/backend/a64/`). Guest Xbox 360 code is still PowerPC; only the **host** architecture changes.

## Prerequisites

1. **Visual Studio 2022** with:
   - Desktop development with C++
   - **MSVC v143 – VS 2022 C++ ARM64/ARM64EC build tools** (`Microsoft.VisualStudio.Component.VC.Tools.ARM64`)
2. **Vulkan SDK** (same as x64 builds)
3. **Python 3** + git submodules initialized

For **cross-compile from x64**, the ARM64 build tools component is required on the same VS install.

## Build directory

| Host | Target | Output dir |
|------|--------|------------|
| x64 | ARM64 (cross) | `Build/Windows/ARM64/` |
| ARM64 | ARM64 (native) | `Build/Windows/ARM64/` |
| ARM64 | x64 (cross) | `Build/Windows/x64/` |

## Commands

From `xenia-phoenix-src` with **vcvars64** active (x64 host) or native ARM64 Developer Prompt:

```powershell
# One-time configure + submodules
python xenia-build.py setup --target-arch arm64

# Release emulator
python xenia-build.py build --target-arch arm64 --config=release --target=xenia-app

# With tests and fuzz tools
python xenia-build.py build --target-arch arm64 --config=release --build-tests --cmake-define XENIA_BUILD_MISC=ON
```

Binary (Release): `Build\Windows\ARM64\Release\xenia_canary.exe` (cross from x64).

## Runtime

- Default JIT: `A64Backend` when built for ARM64 (`cpu=a64` or `cpu=any`).
- Do not pass `cpu=x64` on ARM64 builds (x64 backend is not linked).

```text
xenia_canary.exe --help
xenia_canary.exe path\to\default.xex
```

## Verify script

```powershell
.\tools\tier0\arm64-verify.ps1
.\tools\tier0\arm64-verify.ps1 -SkipCpuTests   # smoke only
```

## Runtime checklist (manual)

[arm64_runtime_checklist.md](arm64_runtime_checklist.md) — Phase 4.3 sign-off on WoA hardware.

## Limitations

- **vmx128-fuzz** x64↔a64 differential on one machine requires two builds; full fuzz sweep is easiest on ARM64 hardware.
- **GPU trace dump tools** link the x64 CPU backend in CMake today; use x64 build for trace tooling.
- **APU** uses scalar conversion on ARM64 (no NEON fast path yet).
- **Linux/macOS ARM64** desktop ports are Phase 4.4+; see [arm64_compat_roadmap.md](../../metacache/plan/arm64_compat_roadmap.md).

## CI

Pull requests run `build-win_arm64.yml` (cross-compile on `windows-2025`). See [arm64_status.md](arm64_status.md).
