# macOS port status

Last updated: **2026-05-16** (Phase 4.4a landing).

## Automated gates (CI)

- [x] `build-macos.yml` on `macos-15` (native arm64)
- [x] `tools/tier0/macos-verify.sh` — `--help` + `xenia-cpu-tests`
- [ ] First green CI run confirmed on GitHub

## Subsystem status

| Subsystem | Build | Runtime verified |
|-----------|-------|------------------|
| CMake / a64 backend | CI | — |
| Cocoa UI + Metal surface | CI | Manual |
| MoltenVK / gpu/vulkan | CI | Manual |
| CPU tests | CI | — |
| SDL audio/HID | CI | Manual |

## Quick commands

See [macos_build.md](macos_build.md) and [metacache/dev/macos_compat_runbook.md](../../metacache/dev/macos_compat_runbook.md).
