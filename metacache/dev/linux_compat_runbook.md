# Linux compatibility runbook (Phoenix)

Linux is **Phase 2** only — see [plan/20_tier_roadmap.md](../plan/20_tier_roadmap.md) § Phase 2. Phase 3 = Android; Phase 4 = ARM64.

## Canonical build (Windows host)

```powershell
cd G:\Dev\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src
.\tools\docker\run-linux-build.ps1              # release + verify
.\tools\docker\run-linux-build.ps1 -Config all  # release, debug, checked
.\tools\docker\run-linux-build.ps1 -SkipVerify  # compile only
```

Logs: `linux_build_<config>.log`

Faster iteration:

```powershell
docker build -t phoenix-linux-build:24.04 -f tools/docker/Dockerfile tools/docker
.\tools\docker\run-linux-build.ps1 -Image phoenix-linux-build:24.04
```

## What verify does (`linux-verify.sh`)

1. Install runtime apt packages (xvfb, SDL2, GTK, Vulkan, ALSA)
2. `xenia_canary --help` under xvfb
3. `scripts/xenia-phoenix-linux.sh --help`
4. **Release only:** build/run `xenia-cpu-tests`; AppImage smoke if artifact exists

Standalone:

```bash
bash tools/docker/linux-verify.sh release post-build
bash tools/docker/linux-verify.sh release smoke-only
```

## CI parity

[Linux_x86.yml](../../xenia-phoenix-src/.github/workflows/Linux_x86.yml) must match Docker:

- `python3 xenia-build.py doctor` before build
- Release: `--build-tests` + `xenia-cpu-tests`
- Smoke: binary + launcher + AppImage
- Submodule paths: `tr -d '\r'` (no `-j` on submodule update)

## Reading build logs

| Pattern | Category | Typical fix |
|---------|----------|-------------|
| `pkg-config ... not found` | configure | apt in `linux-build.sh` / doctor |
| `PPCContext` typedef conflict | compile | include `ppc_context.h`, no forward decl |
| `CMAKE_HOME_DIRECTORY` `G:/` | configure | delete `build/` or use Docker script |
| `pathspec '...?'` | configure | CRLF in `.gitmodules` |

Update [linux_build_blockers.md](../../xenia-phoenix-src/docs/linux_build_blockers.md) when fixing new classes.

## Manual runtime / packaging

- [linux_runtime_checklist.md](../../xenia-phoenix-src/docs/linux_runtime_checklist.md)
- [linux_packaging.md](../../xenia-phoenix-src/docs/linux_packaging.md)
