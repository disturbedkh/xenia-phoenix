# macOS compat runbook

Phase **4.4a** — see [arm64_compat_roadmap.md](../plan/arm64_compat_roadmap.md).

## XeniOS local reference

Sibling repo: `../../XeniOS` (branch `xenios`). Use for read-only diffs and cherry-picks — not a Phoenix submodule. Compare `surface_mac.mm`, `gpu/vulkan`, `kernel` before Mac GPU/UI PRs. Do not port `gpu/metal/` or Qt UI. See [macos_compat_gap_analysis.md](../plan/macos_compat_gap_analysis.md).

## Build

```bash
export VULKAN_SDK=/path/to/macOS
export PATH="$VULKAN_SDK/bin:$PATH"
export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"

cmake -S xenia-phoenix-src -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DXENIA_BUILD_TESTS=ON -DXENIA_BUILD_MISC=ON
cmake --build build --target xenia_canary xenia-cpu-tests --parallel
```

## Verify

```bash
cd xenia-phoenix-src
./tools/tier0/macos-verify.sh
```

## Windows ARM64 (cross, from Windows x64 host)

See [arm64_compat_runbook.md](arm64_compat_runbook.md).
