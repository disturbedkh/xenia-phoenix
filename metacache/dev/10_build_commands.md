# Build commands

Authoritative list. The agent must use these literally; do not improvise.

## Prerequisites (one-time)

**Status as of 2026-05-15 — ALL INSTALLED ✓**

| Tool | Version | Location |
|------|---------|----------|
| Visual Studio 2022 Community | 17.14 (MSVC 14.44.35207) | `C:\Program Files\Microsoft Visual Studio\2022\Community` |
| CMake | 4.3.2 | `C:\Program Files\CMake\bin\cmake.exe` |
| Ninja | 1.13.2 | `C:\Users\khutt\AppData\Local\Microsoft\WinGet\Links\ninja.exe` |
| Vulkan SDK | 1.4.350.0 | `C:\VulkanSDK\1.4.350.0` |
| Python | 3.14.1 | `C:\Users\khutt\AppData\Local\Programs\Python\Python314\python.exe` |
| Git | latest | `C:\Program Files\Git\cmd\git.exe` |

If any tool is missing, reinstall:

```powershell
winget install --id Microsoft.VisualStudio.2022.Community --accept-package-agreements --accept-source-agreements --override "--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --quiet --norestart"
winget install --id Kitware.CMake --accept-package-agreements --silent
winget install --id Ninja-build.Ninja --accept-package-agreements --silent
winget install --id KhronosGroup.VulkanSDK --accept-package-agreements --silent
```

## CRITICAL: always build inside vcvars64

Ninja on Windows requires the MSVC Developer environment (vcvars64) to be active or `stddef.h` and all other SDK headers will be missing. See P-011.

**The canonical build recipe:**

```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$src    = "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"

cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py build"
```

## One-shot bootstrap (fresh clone)

```powershell
# 1. Clone
git clone https://github.com/xenia-canary/xenia-canary --branch canary_experimental --recurse-submodules --shallow-submodules --depth=1 "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"

# 2. Configure + build (from inside vcvars64)
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$src    = "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py setup && python xenia-build.py build"
```

## Incremental build (most common)

```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$src    = "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py build"
```

## Useful CMake flags

| Flag | Purpose |
|------|---------|
| `-DXENIA_BUILD_TESTS=ON` | builds Catch2 unit tests (`xenia-cpu-tests` etc.) |
| `-DXENIA_BUILD_MISC=ON` | builds tools subdir (incl. `vmx128-fuzz`) |
| `-DCMAKE_BUILD_TYPE=Release` | for ninja-style single-config gen |
| `-DXE_OPTION_ENABLE_LOGGING=ON` | extra logging in Release builds |

## Build outputs

| Target | Path (Release) |
|--------|----------------|
| `xenia_canary.exe` | `build\src\xenia\app\Release\xenia_canary.exe` |
| `xenia-cpu-tests.exe` | `build\src\xenia\cpu\testing\Release\xenia-cpu-tests.exe` |
| `vmx128-fuzz.exe` | `build\tools\vmx128_fuzz\Release\vmx128-fuzz.exe` |
| `xenia-gpu-d3d12-trace-dump.exe` | `build\src\xenia\gpu\d3d12\Release\xenia-gpu-d3d12-trace-dump.exe` |
| `xenia-gpu-vulkan-trace-dump.exe` | `build\src\xenia\gpu\vulkan\Release\xenia-gpu-vulkan-trace-dump.exe` |

## Fast incremental loop

```powershell
cmake --build build --config Release --target xenia-cpu-tests --parallel
cmake --build build --config Release --target vmx128-fuzz --parallel
cmake --build build --config Release --target xenia_canary --parallel
```

## Clean

```powershell
Remove-Item -Recurse -Force build
```

## Linux — Docker (canonical)

Requires Docker Desktop running. See [linux_compat_runbook.md](linux_compat_runbook.md).

```powershell
$src = "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
cd $src
.\tools\docker\run-linux-build.ps1
.\tools\docker\run-linux-build.ps1 -Config all
.\tools\docker\run-linux-build.ps1 -SkipVerify   # compile only
```

Log: `linux_build_<config>.log`. Verify step runs smoke + `xenia-cpu-tests` (Release).

Binary (Release): `build\bin\Linux\Release\xenia_canary` (inside Linux container path `/src/build/bin/Linux/Release/xenia_canary`).

Inside container after manual build:

```bash
export CC=clang-20 CXX=clang++-20
./xenia-build.py doctor
./xenia-build.py build --config=release
```

## Windows ARM64 (Phase 4)

Requires **MSVC ARM64 build tools**. See [arm64_compat_runbook.md](arm64_compat_runbook.md).

```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$src    = "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py setup --target-arch arm64 && python xenia-build.py build --target-arch arm64 --config=release --target=xenia-app --build-tests"
```

| Target | Path (Release, cross from x64) |
|--------|--------------------------------|
| `xenia_canary.exe` | `build-arm64\bin\Windows\Release\xenia_canary.exe` |
| `xenia-cpu-tests.exe` | `build-arm64\bin\Windows\Release\xenia-cpu-tests.exe` |

Verify: `.\tools\tier0\arm64-verify.ps1`

## macOS Apple Silicon (Phase 4.4a)

See [macos_compat_runbook.md](macos_compat_runbook.md). Requires `VULKAN_SDK` with MoltenVK ICD.

## Linux ARM64 (Phase 4.4b)

Native on `aarch64` host (e.g. `ubuntu-24.04-arm` CI):

```bash
export CC=clang-20 CXX=clang++-20
./xenia-build.py setup
./xenia-build.py build --config=release --target=xenia-app --build-tests
```

## Android NDK (Phase 3)

See [android_compat_runbook.md](android_compat_runbook.md). Studio: `android/android_studio_project/` (NDK r26c, minSdk 26).

Docker (CI parity):

```bash
docker run --rm -v "$PWD:/src" -w /src ubuntu:24.04 bash tools/docker/android-ndk-build.sh
```

Local NDK:

```bash
export ANDROID_NDK_ROOT=/path/to/android-ndk-r26c
cmake -S . -B build-android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DCMAKE_BUILD_TYPE=Release \
  -DXENIA_BUILD_TESTS=OFF \
  -DXENIA_BUILD_MISC=OFF
cmake --build build-android-arm64 --target xenia-app -j$(nproc)
```

Output: `build-android-arm64/bin/Android/libxenia-app.so`
