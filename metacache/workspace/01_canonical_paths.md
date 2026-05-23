# Canonical paths

## Source of truth

| What | Where |
|------|--------|
| C++ / CMake / CI | `Xenia-Phoenix/xenia-phoenix-src/` |
| Tier0 tools | `xenia-phoenix-src/tools/` (`tier0/`, `vmx128_fuzz/`, `gpu_replay_ci/`, `xma2_diff/`) |
| Game patches | `xenia-phoenix-src/game-patches/` (or submodule path in tree) |
| GPU trace corpus | `xenia-phoenix-src/tests/gpu_traces/` |
| Runtime telemetry | `xenia-phoenix-src/telemetry/` (gitignored) |
| Agent docs | `Xenia-Phoenix/metacache/` |

All tool changes go in `xenia-phoenix-src/tools/` only (no duplicate tools tree under Phoenix).

## Reference snapshot

`../../xenia-canary-canary_experimental/` may lack git/submodules. Use only for diff comparison when `xenia-phoenix-src` is unavailable.

## Paths with spaces

Workspace root: `G:\Xenia-Xenia Canary\`. Quote paths in PowerShell:

```powershell
cd "G:\Xenia-Xenia Canary\Xenia-Phoenix\xenia-phoenix-src"
```

WSL: `/mnt/g/Xenia-Xenia Canary/Xenia-Phoenix/xenia-phoenix-src`
