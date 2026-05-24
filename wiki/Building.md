# Building Phoenix

Authoritative agent commands: [../metacache/dev/10_build_commands.md](../metacache/dev/10_build_commands.md). This page summarizes the Windows path; port builds link to metacache runbooks.

## Prerequisites

| Tool | Notes |
|------|-------|
| Visual Studio 2022 | Desktop development with C++ (MSVC) |
| CMake | On PATH |
| Ninja | On PATH |
| Vulkan SDK | Set `VULKAN_SDK` when building |
| Python 3 | For `xenia-build.py` and tier0 scripts |
| Git | Submodules required for clone |

Install via winget (example):

```powershell
winget install --id Microsoft.VisualStudio.2022.Community --accept-package-agreements --accept-source-agreements --override "--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --quiet --norestart"
winget install --id Kitware.CMake --accept-package-agreements --silent
winget install --id Ninja-build.Ninja --accept-package-agreements --silent
winget install --id KhronosGroup.VulkanSDK --accept-package-agreements --silent
```

## CRITICAL: build inside vcvars64

Ninja on Windows needs the MSVC developer environment. Without `vcvars64.bat`, SDK headers such as `stddef.h` will be missing.

Set variables for your machine:

```powershell
$XeniaRoot = "G:\The Xenia Project"   # adjust
$src       = "$XeniaRoot\Xenia-Phoenix\xenia-phoenix-src"
$vcvars    = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### Canonical incremental build

```powershell
cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py build"
```

### Fresh clone bootstrap

```powershell
git clone https://github.com/disturbedkh/xenia-phoenix.git --branch canary_experimental --recurse-submodules --shallow-submodules --depth=1 "$src"

cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py setup && python xenia-build.py build"
```

## Useful CMake flags

| Flag | Purpose |
|------|---------|
| `-DXENIA_BUILD_TESTS=ON` | Catch2 unit tests (`xenia-cpu-tests`) |
| `-DXENIA_BUILD_MISC=ON` | Tools subdir (`vmx128-fuzz`, etc.) |
| `-DCMAKE_BUILD_TYPE=Release` | Single-config generators |
| `-DXE_OPTION_ENABLE_LOGGING=ON` | Extra logging in Release |

## Build outputs (Release, typical)

| Target | Path |
|--------|------|
| `xenia_canary.exe` | `build\src\xenia\app\Release\xenia_canary.exe` |
| `xenia-cpu-tests.exe` | `build\src\xenia\cpu\testing\Release\xenia-cpu-tests.exe` |
| `vmx128-fuzz.exe` | `build\tools\vmx128_fuzz\Release\vmx128-fuzz.exe` |

(Paths may also appear under `build\bin\Windows\Release\` depending on generator — check your tree after first build.)

## Fast incremental targets

```powershell
cd $src
cmake --build build --config Release --target xenia-cpu-tests --parallel
cmake --build build --config Release --target vmx128-fuzz --parallel
cmake --build build --config Release --target xenia_canary --parallel
```

## Clean

```powershell
Remove-Item -Recurse -Force "$src\build"
```

## Other platforms

| Platform | Doc |
|----------|-----|
| Linux (Docker) | [../metacache/dev/linux_compat_runbook.md](../metacache/dev/linux_compat_runbook.md) |
| Windows ARM64 | [../metacache/dev/arm64_compat_runbook.md](../metacache/dev/arm64_compat_runbook.md) |
| macOS | [../metacache/dev/macos_compat_runbook.md](../metacache/dev/macos_compat_runbook.md) |
| Android | [../metacache/dev/android_compat_runbook.md](../metacache/dev/android_compat_runbook.md) |

General Xenia build notes: [../xenia-phoenix-src/docs/building.md](../xenia-phoenix-src/docs/building.md) · Tier 0 bootstrap: [../xenia-phoenix-src/docs/TIER0_BUILD.md](../xenia-phoenix-src/docs/TIER0_BUILD.md)

## Known pitfalls

See [../metacache/dev/60_known_pitfalls.md](../metacache/dev/60_known_pitfalls.md) (e.g. P-011 vcvars, Vulkan SDK path).

## Deeper detail (agents)

- [../metacache/dev/10_build_commands.md](../metacache/dev/10_build_commands.md)
