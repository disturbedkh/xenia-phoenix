# macOS build (Apple Silicon)

Requires **macOS 15+**, **Xcode Command Line Tools**, **CMake**, **Ninja**, and the **Vulkan SDK** (MoltenVK).

## Prerequisites

```bash
xcode-select --install
brew install cmake ninja
# Install Vulkan SDK from https://vulkan.lunarg.com/ (macOS)
export VULKAN_SDK=/path/to/macOS
export PATH="$VULKAN_SDK/bin:$PATH"
export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"
```

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DXENIA_BUILD_TESTS=ON -DXENIA_BUILD_MISC=ON
cmake --build Build/Windows/x64/vs --target xenia_canary xenia-cpu-tests --parallel
```

Output: `Build/macOS/ARM64/Release/xenia_canary`

## Verify

```bash
./tools/tier0/macos-verify.sh
```

## Runtime

Default GPU: Vulkan via MoltenVK. CPU backend: `a64` on Apple Silicon.

See [macos_runtime_checklist.md](macos_runtime_checklist.md).
