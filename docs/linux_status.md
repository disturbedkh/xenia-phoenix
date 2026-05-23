# Linux port status (Phase 2)

Last updated: **2026-05-23**. Roadmap: [metacache/plan/20_tier_roadmap.md](../../metacache/plan/20_tier_roadmap.md) § Phase 2.

## Phase 2.2–2.4 — Automated (done)

- [x] Release build (Ubuntu 24.04, clang-20) — CI green [run 26339810511](https://github.com/disturbedkh/xenia-phoenix/actions/runs/26339810511) (2026-05-23)
- [x] `xenia-build.py doctor` in CI and Docker
- [x] `linux-verify.sh`: smoke, launcher, `xenia-cpu-tests` (Release)
- [x] Debug / Checked in CI (smoke)
- [x] AppImage build + `--help` smoke in CI
- [x] `run-linux-build.ps1 -Config all`; optional `Dockerfile`

## Phase 2.5–2.7 — Manual (open)

- [ ] [linux_runtime_checklist.md](linux_runtime_checklist.md) signed off on bare metal
- [ ] [linux_packaging.md](linux_packaging.md) — AppImage on clean VM with window
- [ ] Smoke titles — [60_smoke_titles.md](../../metacache/plan/60_smoke_titles.md) Linux column

## Subsystem status

| Subsystem | Build (2.2–2.4) | Runtime (2.5) |
|-----------|-----------------|---------------|
| CMake / deps | CI + Docker | — |
| CPU tests | CI + Docker verify | — |
| UI / GTK | Compiles | Checklist 2.5 |
| Vulkan | Compiles | Checklist 2.5 |
| Audio / HID | Compiles | Checklist 2.5 |

## Quick commands

```powershell
.\tools\docker\run-linux-build.ps1
```

```bash
./scripts/xenia-phoenix-linux.sh --help
```
