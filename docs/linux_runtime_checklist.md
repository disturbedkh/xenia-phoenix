# Linux runtime verification checklist (Phase 2.5)

Manual sign-off on **bare-metal Ubuntu 24.04** (or equivalent). Automated gates are in CI/Docker (`linux-verify.sh`); this doc covers subsystems that need a display, audio device, or game content.

## Prerequisites

```bash
./scripts/xenia-phoenix-linux.sh --help   # or Build/Linux/x64/Release/xenia_canary
export GDK_BACKEND=x11
# VULKAN_SDK set by launcher if ~/vulkan-sdk exists; else install LunarG SDK
```

| Distro | Equivalent packages (doctor / build deps) |
|--------|-------------------------------------------|
| Ubuntu 24.04 | `libgtk-3-dev`, `libsdl2-dev`, `libasound2-dev`, `libx11-xcb-dev` — see `xenia-build.py doctor` |
| Arch | `gtk3`, `sdl2`, `alsa-lib`, `libx11`, `vulkan-devel`, `spirv-tools` |
| Fedora | `gtk3-devel`, `SDL2-devel`, `alsa-lib-devel`, `libX11-devel`, `vulkan-loader-devel` |

## Checklist

| ID | Subsystem | Steps | Pass |
|----|-----------|-------|------|
| 3.1 | GTK / X11 window | Launch emulator; main window appears; ImGui menu opens | [ ] |
| 3.2 | Vulkan | No SPIR-V tool errors in log; menu/background renders | [ ] |
| 3.3 | Audio ALSA | Default output device plays (in-game or UI sound if applicable) | [ ] |
| 3.4 | Audio SDL | Fallback if ALSA unavailable | [ ] |
| 3.5 | HID SDL | Controller detected in settings; input in menu | [ ] |
| 3.6 | Headless path | `xvfb-run -a ./xenia_canary --headless` (no storage dialog hang) | [ ] |
| 3.7 | Launcher | `./scripts/xenia-phoenix-linux.sh` sets `GDK_BACKEND` + `VULKAN_SDK` | [ ] |

## AppImage (Phase 2.7)

On a **clean** Ubuntu 24.04 VM (no dev packages):

| Step | Pass |
|------|------|
| `chmod +x xenia_canary_linux.AppImage && ./xenia_canary_linux.AppImage --help` | [ ] |
| Window launch from AppImage (X11 session) | [ ] |
| `ldd` / run shows Vulkan + SDL + GTK libs bundled or documented | [ ] |

## Blockers

File new rows in [linux_build_blockers.md](linux_build_blockers.md) (B23+) with log excerpts. Do not gate CI on game ISOs.

## Smoke titles (Phase 2.6)

See [metacache/plan/60_smoke_titles.md](../../metacache/plan/60_smoke_titles.md) Linux column.
