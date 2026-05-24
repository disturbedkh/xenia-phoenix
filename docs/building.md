# Building

**Phoenix:** canonical Windows commands and vcvars requirements are in [../../wiki/Building.md](../../wiki/Building.md) and [../../metacache/dev/10_build_commands.md](../../metacache/dev/10_build_commands.md).

You must have a 64-bit machine for building and running the project. Always
run your system updater before building and make sure you have the latest
drivers.

## Setup

### Windows

* Windows 10 or later
* [Visual Studio 2022](https://www.visualstudio.com/downloads/)
* CMake 3.10+ (or C++ CMake tools for Windows)
* Windows 11 SDK version 10.0.22000.0 (for Visual Studio 2022, this or any newer version)
* [Python 3.6+ 64-bit](https://www.python.org/downloads/)
  * Ensure Python is in PATH.
* [Vulkan SDK](https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe)
  * The build script will automatically detect it if installed at `C:\VulkanSDK`

```
git clone https://github.com/xenia-canary/xenia-canary.git
cd xenia-canary
xb setup

# Build on command line (add --config=release for release):
xb build


# Pull latest changes, rebase, update submodules, and run premake:
xb pull

# Run premake and open Visual Studio (run the 'xenia-app' project):
xb devenv

# Run premake to update the sln/vcproj's:
xb premake

# Format code to the style guide:
xb format
```
<!--
# Remove intermediate files and build outputs (doesn't work on Linux):
xb clean

# Check for lint errors with clang-format:
xb lint

# Run the style checker on all code:
xb style

# Remove all build/ output and do a hard git reset:
xb nuke

# Runs the clang-tidy checker on all code:
xb tidy


## Testing:

# Generate tests:
xb gentests

# Run tests:
xb test

# Run GPU tests:
xb gputest


## Other:

# Generate SPIR-V binaries and header files:
xb genspirv
-->

#### Debugging

VS behaves oddly with the debug paths. Open the 'xenia-app' project properties
and set the 'Command' to `$(SolutionDir)$(TargetPath)` and the
'Working Directory' to `$(SolutionDir)..\..`. You can specify flags and
the file to run in the 'Command Arguments' field (or use `--flagfile=flags.txt`).

By default logs are written to a file with the name of the executable. You can
override this with `--log_file=log.txt`.

If running under Visual Studio and you want to look at the JIT'ed code
(available around 0xA0000000) you should pass `--emit_source_annotations` to
get helpful spacers/movs in the disassembly.

### Linux

Linux support is experimental. Use **Clang 20** (matching CI). GCC is not supported
for production builds.

* Normal building via `xb build` uses CMake+Ninja.
* Run `./xenia-build.py doctor` (or `xb doctor`) to verify dependencies before building.
* Docker build on Windows: `tools/docker/run-linux-build.ps1` (see Project Phoenix `metacache/dev/linux_compat_runbook.md`).
* See [linux_status.md](linux_status.md) and [linux_build_blockers.md](linux_build_blockers.md).

Environment variables:

  Name  | Default Value
  ----- | -------------
  `CC`  | `clang`
  `CXX` | `clang++`

#### Ubuntu 22.04 / 24.04 (recommended)

This matches [.github/workflows/Linux_x86.yml](../.github/workflows/Linux_x86.yml):

```sh
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
sudo apt-add-repository "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-20 main"
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build python3 pkg-config git \
  mesa-vulkan-drivers valgrind libc++-dev libc++abi-dev \
  libgtk-3-dev liblz4-dev libsdl2-dev libvulkan-dev libx11-xcb-dev \
  libasound2-dev libfontconfig1-dev libfuse2 spirv-tools \
  clang-20 lld-20 llvm-20

export CC=clang-20 CXX=clang++-20
```

#### Build

```sh
cd xenia-phoenix-src
git submodule update --init --depth=1 $(grep -oP '(?<=path = ).+' .gitmodules | grep -v DirectXShaderCompiler)

./xenia-build.py doctor
./xenia-build.py build --config=release
./build/bin/Linux/Release/xenia_canary --help
```

Optional launcher:

```sh
./scripts/xenia-phoenix-linux.sh
```

**Vulkan SDK (shader compile + runtime SPIR-V tools)**

```sh
spirv-opt --version
./xenia-build.py doctor
```

If the system `spirv-tools` package is too old, install the
[LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home) and set `VULKAN_SDK`:

```sh
wget -qO vulkan-sdk.tar.xz https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.xz
mkdir -p ~/vulkan-sdk
tar -xf vulkan-sdk.tar.xz -C ~/vulkan-sdk
rm vulkan-sdk.tar.xz
export VULKAN_SDK=$HOME/vulkan-sdk/$(ls ~/vulkan-sdk)/x86_64
export PATH="$VULKAN_SDK/bin:$PATH"
```

Add the `export` lines to your shell profile. The build falls back without
`--canonicalize-ids` if needed, but the LunarG SDK is still recommended.

### Android

Experimental. Requires Vulkan 1.0+ on device. See [android_status.md](android_status.md).

**Android Studio:** open `android/android_studio_project/`, sync Gradle, build `arm64-v8a`.

**NDK + CMake (command line):**

```sh
export ANDROID_NDK_ROOT=/path/to/ndk/25.2.9519653
cmake -S . -B build-android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DCMAKE_BUILD_TYPE=Release \
  -DXENIA_BUILD_TESTS=OFF \
  -DXENIA_BUILD_MISC=OFF
cmake --build build-android-arm64 --target xenia-app -j$(nproc)
```

Produces `libxenia-app.so` (loads from the APK). CI: [.github/workflows/Android_arm64.yml](../.github/workflows/Android_arm64.yml).

## Running

To make life easier you can set the program startup arguments in your IDE to something like `--log_file=stdout /path/to/Default.xex` to log to console rather than a file and start up the emulator right away.
