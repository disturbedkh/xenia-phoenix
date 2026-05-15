# Tier 0 build bootstrap (PC-only)

This document matches MVP0 of the Tier 0 execution plan: get a repeatable **Release** and **Checked** (ASan) build with tests and misc tools enabled.

## Prerequisites (Windows x64)

1. **Visual Studio 2022** — workload *Desktop development with C++*, component *C++ CMake tools for Windows*.
2. **Windows 11 SDK** — 10.0.22000.0 or newer (see [building.md](building.md)).
3. **Python 3.10+** (64-bit) on `PATH`.
4. **Vulkan SDK** — install to `C:\VulkanSDK\<version>` so shader tools (`glslangValidator`, `spirv-opt`) resolve (see [building.md](building.md)).

## One-shot script

From the repo root (`xenia-canary`):

```powershell
.\tools\tier0\bootstrap.ps1
```

Optional flags:

```powershell
.\tools\tier0\bootstrap.ps1 -Preset vs -Configuration Release -BuildTests -BuildMisc
.\tools\tier0\bootstrap.ps1 -RunCpuTests   # after a successful build
```

## Manual CMake (Ninja Multi-Config)

```powershell
git submodule update --init --recursive
cmake --preset default -DXENIA_BUILD_TESTS=ON -DXENIA_BUILD_MISC=ON
cmake --build build --config Release --parallel
cmake --build build --config Checked --parallel
```

Outputs are under `build/bin/Windows/` (see root `CMakeLists.txt` `CMAKE_RUNTIME_OUTPUT_DIRECTORY`).

## Binaries used by Tier 0

| Binary | When built |
|--------|------------|
| `xenia-canary.exe` | Default app target |
| `xenia-cpu-tests.exe` | `XENIA_BUILD_TESTS=ON` |
| `xenia-gpu-d3d12-trace-dump.exe` | `XENIA_BUILD_MISC=ON`, Windows |
| `xenia-gpu-vulkan-trace-dump.exe` | `XENIA_BUILD_MISC=ON`, non-Apple |
| `vmx128-fuzz.exe` | `XENIA_BUILD_MISC=ON` |

## Build timing

Record clean vs incremental times on your machine (for CI cache decisions):

| Step | Time (fill in) |
|------|------------------|
| Clean configure (default preset) | |
| Clean build Release | |
| Incremental after one `.cc` change | |

## AArch64 JIT (optional)

To fuzz **x64 vs a64** backends you need an **ARM64 Windows** host or VM with VS ARM64 toolchain. On pure x64 hosts, run guest PPC tests via `TestGuestPpcBlock` and `vmx128-fuzz` on x64 only; defer a64 differential until ARM hardware is available.
