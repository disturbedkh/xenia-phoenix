# Phoenix build matrix

Canonical layout: `Build/<OS>/<arch>/<Config>/` (Android omits `<arch>`).

Resolve paths: `python tools/build/xenia_paths.py bin --config Release [--target-arch arm64] [--os Android]`
or `python xenia-build.py where --config release`.

| OS | Arch | Config | CI workflow | Local repro |
|----|------|--------|-------------|-------------|
| Windows | x64 | Release | `Windows_x86.yml`, `tier0-differential.yml` | `python xenia-build.py build --config=release` |
| Windows | x64 | Debug | (local) | `python xenia-build.py build --config=debug` |
| Windows | x64 | Checked | `tier0-checked.yml` (nightly) | `python xenia-build.py build --config=checked --target tests` |
| Windows | ARM64 | Release | `build-win_arm64.yml` | `python xenia-build.py build --target-arch arm64 --config=release --target app --build-tests` |
| Linux | x64 | Release | `Linux_x86.yml` | `CC=clang-20 CXX=clang++-20 python3 xenia-build.py build --config=release` |
| Linux | x64 | Debug | `Linux_x86.yml` (debug input) | `python3 xenia-build.py build --config=debug` |
| Linux | x64 | Checked | `Linux_x86.yml` (checked input) | `python3 xenia-build.py build --config=checked` |
| Linux | ARM64 | Release | `build-linux_arm64.yml` | Native aarch64: `python3 xenia-build.py build --config=release`; from x64: `tools/docker/run-linux-build.ps1 -Arch arm64` |
| macOS | ARM64 | Release | `build-macos.yml` | `cmake -S . -B Build/macOS/ARM64 -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build Build/macOS/ARM64 --target xenia_canary` |
| Android | (arm64-v8a) | Release | `Android_arm64.yml` | `python xenia-build.py build --target-os android --target app` or `tools/docker/android-ndk-build.sh` |

## Target aliases

`--target` accepts friendly aliases (auto-enable tests/misc as needed):
`app` -> `xenia-app`; `tests` -> `xenia-cpu-tests xenia-base-tests xenia-kernel-tests`;
`fuzz` -> `vmx128-fuzz`; `trace` -> `xenia-gpu-d3d12-trace-dump`; `xma2` -> `xma2-diff`.

## Universal build (the holy grail)

A change can be checked across every cell before push:

- **`python xenia-build.py matrix [--config C] [--cells <subset>] [--build-only]`** — build + check
  every *locally* buildable cell and print an aggregated OK/FAIL/SKIP table.
  Cells: `windows-x64`, `windows-arm64`, `android`, `linux-x64`, `linux-arm64`, `macos`.
  Cells whose toolchain/host is unavailable are SKIPPED (macOS is always SKIP off a
  macOS host). Linux cells run in Docker (`linux-arm64` via buildx/qemu).
- **`python xenia-build.py ci [--ref <branch>] [--no-watch]`** — dispatch the GitHub Actions
  `Orchestrator.yml` on your branch and watch it green/red. This is the only way to
  validate **macOS** (and the authoritative full-matrix result). Requires `gh` + `gh auth login`.

Relationship: `xb preflight` (host fast gate) is a subset of `xb matrix` (all local cells);
`xb ci` covers the remainder (esp. macOS) = the full GitHub matrix.

## Tier 0 gates (Windows x64 Release)

| Gate | CI (PR-blocking) | Local |
|------|------------------|-------|
| path guard | `Lint.yml` | `python tools/build/check_build_paths.py` |
| clang-format lint | `Lint.yml` | `python xenia-build.py lint` |
| commit-message `[Tag]` | `Check_commit_message.yml` | covered by `xb preflight` |
| CPU tests | `tier0-differential.yml` | `Build/Windows/x64/Release/xenia-cpu-tests.exe` |
| vmx128 50k | `tier0-differential.yml` | same dir / `vmx128-fuzz.exe` |
| GPU replay | `tier0-differential.yml` | `python tools/gpu_replay_ci/run.py --build-dir Build/Windows/x64 --cross-path d3d12` |

Nightly / non-blocking (`nightly.yml`, schedule + manual):

| Gate | Local |
|------|-------|
| Checked/ASan cpu-tests | `python xenia-build.py build --config=checked --target tests` then run them |
| clang-tidy (changed files) | `python xenia-build.py tidy --changed --base origin/canary_experimental` |
| vmx128 1M (weekly) | `tools/tier0/run_vmx128_full_sweep.ps1` |

## Preflight

`python xenia-build.py preflight` (alias `green`) mirrors the PR-blocking gates locally
(path guard, clang-format lint, commit-message `[Tag]`, build, cpu-tests, vmx128 50k,
GPU replay, phoenixctl tests). See `metacache/plan/build_system_roadmap.md`.
