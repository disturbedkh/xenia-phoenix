# Canonical paths

## Source of truth

| What | Where |
|------|--------|
| C++ / CMake / CI | `Xenia-Phoenix/xenia-phoenix-src/` |
| Tier0 tools | `xenia-phoenix-src/tools/` (`tier0/`, `vmx128_fuzz/`, `gpu_replay_ci/`, `xma2_diff/`) |
| Game patches | `Xenia-Phoenix/game-patches/` (sibling clone; see categorize_patches.py) |
| GPU trace corpus | `xenia-phoenix-src/tests/gpu_traces/` |
| Runtime telemetry | `xenia-phoenix-src/telemetry/` (gitignored) |
| Agent docs | `Xenia-Phoenix/metacache/` |

All tool changes go in `xenia-phoenix-src/tools/` only (no duplicate tools tree under Phoenix).

## Reference snapshots (sibling clones)

| Clone | Path | Sync |
|-------|------|------|
| Canary | `../../xenia-canary/` | `.\scripts\sync-upstreams.ps1 -Repo xenia-canary` |
| Master | `../../xenia-master/` | `-Repo xenia-master` |
| Edge | `../../Xenia-Edge/` | `-Repo xenia-edge` |
| XeniOS | `../../XeniOS/` | `-Repo xenios` |

Use for diff/triage when comparing Phoenix against upstream forks. Config: [`workspace/upstreams.yaml`](../../../workspace/upstreams.yaml).

## Paths with spaces

Workspace root: `G:\Dev\The Xenia Project\`. Canonical variable:

```powershell
$XeniaRoot = "G:\Dev\The Xenia Project"
```

Quote paths in PowerShell:

```powershell
cd "G:\Dev\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
```

WSL: `/mnt/g/Dev/The Xenia Project/Xenia-Phoenix/xenia-phoenix-src`
