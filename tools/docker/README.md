# Linux Docker build (Phoenix)

## Prerequisites

- Docker Desktop running (`docker info` shows Server section)
- Source: `xenia-phoenix-src` with git submodules (script initializes inside container)

## Windows (canonical)

```powershell
cd G:\Xenia-Xenia Canary\Xenia-Phoenix\xenia-phoenix-src
.\tools\docker\run-linux-build.ps1
.\tools\docker\run-linux-build.ps1 -Config debug
.\tools\docker\run-linux-build.ps1 -Config all    # release + debug + checked
.\tools\docker\run-linux-build.ps1 -SkipVerify    # build only, no smoke/tests
```

Log: `linux_build_<config>.log` in repo root.

Each successful run also executes `linux-verify.sh` (smoke, launcher, `xenia-cpu-tests` on Release).

## Pre-baked image (faster rebuilds)

```powershell
docker build -t phoenix-linux-build:24.04 -f tools/docker/Dockerfile tools/docker
.\tools\docker\run-linux-build.ps1 -Image phoenix-linux-build:24.04
```

## Manual

```powershell
docker run --rm -v "G:\...\xenia-phoenix-src:/src:rw" -w /src ubuntu:24.04 `
  bash -lc "sed -i 's/\r$//' tools/docker/linux-build.sh && bash tools/docker/linux-build.sh release"
```

Paths with spaces must be quoted. WSL fallback: `/mnt/g/Xenia-Xenia Canary/Xenia-Phoenix/xenia-phoenix-src`.

## Verify only (after build)

```bash
bash tools/docker/linux-verify.sh release post-build
bash tools/docker/linux-verify.sh release smoke-only
```

## Smoke test (standalone)

```bash
docker run --rm -v "...:/src:rw" -w /src ubuntu:24.04 bash -c '
  apt-get update -qq && apt-get install -y -qq xvfb libsdl2-2.0-0 libgtk-3-0 libvulkan1 mesa-vulkan-drivers libasound2t64 libfontconfig1 libfuse2 &&
  export GDK_BACKEND=x11 &&
  xvfb-run -a ./build/bin/Linux/Release/xenia_canary --help
'
```

Runtime checklist: [docs/linux_runtime_checklist.md](../../docs/linux_runtime_checklist.md).
