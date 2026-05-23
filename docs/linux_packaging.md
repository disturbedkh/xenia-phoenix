# Linux packaging (Phase 2.7 — AppImage)

## CI artifact

Release workflow produces `artifacts/release/xenia_canary_linux.AppImage` via `linuxdeploy` in [.github/workflows/Linux_x86.yml](../.github/workflows/Linux_x86.yml).

Post-build CI smoke: `xvfb-run -a ./xenia_canary_linux.AppImage --help`.

## Manual validation (clean VM)

1. Copy AppImage to Ubuntu 24.04 without dev toolchains installed.
2. `chmod +x xenia_canary_linux.AppImage`
3. `./xenia_canary_linux.AppImage --help` (headless: use `xvfb-run -a` if no display).
4. Launch with a real X11 session for window test (see [linux_runtime_checklist.md](linux_runtime_checklist.md)).

## SPIR-V tools at runtime

If validation fails with missing `libSPIRV-Tools-shared.so`, set `LD_LIBRARY_PATH` to a LunarG Vulkan SDK `lib` directory or use [scripts/xenia-phoenix-linux.sh](../scripts/xenia-phoenix-linux.sh) which sets `VULKAN_SDK` when `~/vulkan-sdk` exists.

Audit bundled libs: `ldd xenia_canary_linux.AppImage` after extraction (`--appimage-extract`).

## Deferred

Flatpak and AUR packaging are not maintained in this tree until AppImage sign-off is complete.
