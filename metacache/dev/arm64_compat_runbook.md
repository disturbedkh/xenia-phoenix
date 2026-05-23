# ARM64 compat runbook (Windows WoA)

Phase **4** only — see [plan/20_tier_roadmap.md](../plan/20_tier_roadmap.md). Not Linux Phase 2, not Android Phase 3.

## Prerequisites

Visual Studio 2022 with **MSVC ARM64 build tools**. Verify:

```powershell
Get-ChildItem "C:\Program Files\Microsoft Visual Studio\2022\*\VC\Tools\MSVC\*\bin\Hostx64\arm64\cl.exe" -ErrorAction SilentlyContinue
```

If empty, install via Visual Studio Installer → **MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools**.

## Build (canonical)

Always use **vcvars64** on an x64 host, then:

```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$src    = "G:\Dev\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py setup --target-arch arm64 && python xenia-build.py build --target-arch arm64 --config=release --target=xenia-app --build-tests"
```

Output: `build-arm64\bin\Windows\Release\xenia_canary.exe`

## Verify

```powershell
cd $src
.\tools\tier0\arm64-verify.ps1
.\tools\tier0\arm64-verify.ps1 -SkipCpuTests
```

## CPU tests

```powershell
cd build-arm64\bin\Windows\Release
.\xenia-cpu-tests.exe
```

On x64 host, tests run via WoA x64 emulation of ARM64 binaries (Windows 11+).

## vmx128-fuzz (ARM64 hardware recommended)

```powershell
python xenia-build.py build --target-arch arm64 --config=release --cmake-define XENIA_BUILD_MISC=ON
cd build-arm64\bin\Windows\Release
.\vmx128-fuzz.exe --vmx128_fuzz_iters=50000
```

## Clean ARM64 build tree

```powershell
Remove-Item -Recurse -Force build-arm64
```

## CI

Workflow: `.github/workflows/build-win_arm64.yml`. Triggered from Orchestrator on every PR.

## Runtime smoke (manual)

1. `xenia_canary.exe --help`
2. Boot a legally owned homebrew/smoke title
3. Confirm D3D12, audio, controller for 5+ minutes
4. Log result in [60_smoke_titles.md](../plan/60_smoke_titles.md) WoA column
