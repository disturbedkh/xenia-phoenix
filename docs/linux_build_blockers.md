# Linux build blockers matrix

Tracked work for [issue #549](https://github.com/xenia-canary/xenia-canary/issues/549) and the Linux compatibility plan.

**Legend:** `category` = configure | third-party | compile | link | runtime

| ID | Category | File / area | Symptom | Fix | Phase |
|----|----------|-------------|---------|-----|-------|
| B01 | configure | `docs/building.md` vs CI | Local apt installs `clang-19`; CI uses `clang-20` | Align docs + `xb doctor` with CI (`clang-20`, `lld-20`) | 1–2 |
| B02 | configure | `spirv-opt` (system package) | Shader compile fails: unknown `--canonicalize-ids` | Install LunarG Vulkan SDK **or** use `compile_shader_spirv.py` fallback without that flag | 1 |
| B03 | configure | `VULKAN_SDK` at runtime | Binary builds but SPIR-V validation fails at startup | Export `VULKAN_SDK` (see `building.md`); use `scripts/xenia-canary-linux.sh` | 3–4 |
| B04 | configure | Git submodules | Empty `third_party/*` → configure/compile errors | `git submodule update --init` (Linux skips `DirectXShaderCompiler`) | 0–1 |
| B05 | configure | `snappy` / `zlib-ng` | Missing generated headers on fresh tree | `execute_process` cmake configure in `third_party/CMakeLists.txt` (auto) | 1 |
| B06 | configure | GTK3 / SDL2 / ALSA | `pkg-config` not found | Install `libgtk-3-dev`, `libsdl2-dev`, `libasound2-dev` | 1–2 |
| B07 | configure | X11 / XCB | Undefined references to `X*` / `xcb_*` | `find_package(X11)` + `pkg_check_modules(XCB …)` in root `CMakeLists.txt` | 1 |
| B08 | compile | Clang `-Werror` | New warnings fail Linux-only targets | Fix warnings or narrow `-Werror` in `xe_target_defaults` | 1 |
| B09 | link | `xe::Socket` | Would link-fail if `socket_win.cc` were included without posix peer | Added `socket_posix.cc` stubs; not used by main app today | 1 |
| B10 | runtime | Wayland default | GTK window fails or no Vulkan surface | `GDK_BACKEND=x11` in `windowed_app_main_posix.cc` (already set) | 3 |
| B11 | runtime | Display / GPU | “Does nothing” after build (issue #549) | Need X11/Wayland session, Vulkan drivers, `VULKAN_SDK` | 3–7 |
| B12 | runtime | `spirv_tools_context.cc` | Loads `$VULKAN_SDK/bin/libSPIRV-Tools-shared.so` | Same as B03 | 4 |
| B13 | third-party | `discord-rpc` | Unix socket path | Verified Linux sources in `third_party/CMakeLists.txt` | 1 |
| B14 | third-party | `FFmpeg` | XMA decode | Submodule branch `xmaframes`; needs init | 1 |
| B15 | compile | `__gnu_linux__` vs Android | `#error Unsupported target OS` if macro missing | Use `__linux__` fallback in `platform.h` | 1 |
| B16 | configure | `.gitmodules` CRLF | `pathspec 'third_party/foo?'` in Docker | `tr -d '\r'` on submodule paths in `linux-build.sh` | 2 |
| B17 | configure | `git submodule -j` | `usage: git submodule` on Ubuntu git 2.43 | Omit `-j` on `submodule update` | 2 |
| B18 | configure | `xenia-build.py` CRLF shebang | `/usr/bin/env: 'python3\r'` | `sed` CRLF + invoke `python3 xenia-build.py` in Docker | 2 |
| B19 | configure | `DoctorCommand` order | `NameError: Command is not defined` at import | Define `DoctorCommand` after `class Command` | 2 |
| B20 | configure | Windows `build/CMakeCache.txt` | Source path mismatch `G:/…` vs `/src` | Remove cache when `CMAKE_HOME_DIRECTORY` is Windows drive | 2 |
| B21 | compile | `stub_trace.h` | `PPCContext` typedef vs forward decl | Include `ppc_context.h`; no `struct PPCContext` forward decl | 2 |
| B22 | compile | `kernel_module.cc` | `.c_str()` on `const char*` export name | Pass `export_entry->name` as `string_view` | 2 |
| B23 | compile | `apu/util/apu_trace.h` | `unknown type name 'uint32_t'` on Linux Clang | `#include <cstdint>` | 2.5 |
| B24 | link | `sha256::SHA256` duplicate | `sha256.cpp` in both `xenia-apu` and `xboxkrnl_crypt.cc` | Compile sha256 once (kernel TU only) | 2.5 |
| B25 | verify | `--help` without TTY | Smoke hangs in Docker/xvfb (`SDL_ShowSimpleMessageBox`) | `script -qefc '… --help' /dev/null` in `linux-verify.sh` + CI | 2.4 |
| B26 | verify | `xenia-cpu-tests` target missing | `ninja: unknown target` when `XENIA_BUILD_TESTS` off | Drop `--no_premake`; re-run cmake with `--build-tests` | 2.4 |
| B27 | verify | `test -- xenia-cpu-tests` | Runs default trio; `Unable to find xenia-base-tests` | Use `--target xenia-cpu-tests` (args after `--` go to Catch2) | 2.4 |
| B28 | compile | `obs_event.cc` | `-Wdangling-assignment-gsl`: `fmt::format` temp assigned to `std::string_view` | Hoist format result to `std::string` local before assigning `ev.detail` | 2.2 |
| B29 | configure | ARM64 CI workflow | LTO link: gold plugin LLVM 17 vs clang-20 bitcode | Pin `lld`/`ld.lld`/`llvm-ar`/`llvm-ranlib`/`llvm-nm` via `update-alternatives` + `apt.llvm.org` | 4.4b |
| B30 | compile | `xenia-apu` + Linux XMP | `SDL.h` not found compiling `audio_media_player.cc` | Link `SDL2` on Linux in `apu/CMakeLists.txt` (EDGE-PORT-A-2) | 2.2 |
| B31 | configure | ARM64 CI + doctor | System `spirv-opt` lacks `--canonicalize-ids`; LunarG tarball has no aarch64 tree | `doctor` step `continue-on-error: true`; build uses compile_shader_spirv fallback (B02) | 4.4b |
| B32 | verify | Checked smoke + LSAN | `--help` smoke exits 1 on LeakSanitizer GTK/SDL leaks | `LSAN_OPTIONS=detect_leaks=0` in CI smoke step | 2.3 |
| B34 | configure | ARM64 shader compile | `glslangValidator` missing (no LunarG aarch64 SDK) | Install `glslang-tools` apt package in `build-linux_arm64.yml` | 4.4b |

## Platform source inventory (`*_win.cc` → Linux peer)

| Windows | Linux / POSIX peer | Notes |
|---------|-------------------|--------|
| `base/socket_win.cc` | `base/socket_posix.cc` | Stub; guest sockets use BSD APIs in `kernel/xsocket.cc` |
| `base/*_win.cc` | `*_posix.cc` / `*_gnulinux.cc` | Via `xe_platform_sources` |
| `cpu/stack_walker_win.cc` | `stack_walker_posix.cc` | OK |
| `ui/*_win.cc` | `*_gtk.cc` / `windowed_app_main_posix.cc` | OK |
| `cpu/backend/x64/x64_code_cache_win.cc` | (posix variant in same dir) | Check `xe_platform_sources` |

## Repro commands (Ubuntu 24.04)

```powershell
# Windows host (canonical)
.\tools\docker\run-linux-build.ps1
```

```sh
# Inside Ubuntu 24.04 container after build
export GDK_BACKEND=x11
apt-get install -y xvfb libsdl2-2.0-0 libgtk-3-0 libvulkan1 mesa-vulkan-drivers libasound2t64
xvfb-run -a script -qefc "./build/bin/Linux/Release/xenia_canary --help" /dev/null
```

**2026-05-16 log excerpt (success):** `Build OK: /src/build/bin/Linux/Release/xenia_canary (16546056 bytes)`

Log files (when captured): `linux_build_release.log`, `linux_build_debug.log`, `linux_build_checked.log`.
