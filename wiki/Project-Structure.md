# Project structure

How the Phoenix monorepo is organized on disk.

## Top level (`Xenia-Phoenix/`)

| Path | Role |
|------|------|
| `xenia-phoenix-src/` | **Canonical build tree** — C++, CMake, CI, tools |
| `metacache/` | Agent memory — strategy, RE, runbooks (separate git branch) |
| `wiki/` | Human documentation (this tree) |
| `game-patches/` | Per-title TOML patches (sibling clone; debt ledger source) |
| `README.md` | Project landing page |

## Git remotes (`xenia-phoenix-src`)

| Remote | URL | Use |
|--------|-----|-----|
| **origin** | `https://github.com/disturbedkh/xenia-phoenix.git` | Push Phoenix work |
| **upstream** | `https://github.com/xenia-canary/xenia-canary.git` | Rebase / merge base |

Branch: `canary_experimental` (both Phoenix and Canary reference).

## Sibling clones (read-only context)

These live next to `Xenia-Phoenix/` under the workspace root — not second copies of Phoenix docs.

| Path | Upstream | Role |
|------|----------|------|
| `xenia-canary/` | xenia-canary/xenia-canary | Reference Canary snapshot |
| `xenia-master/` | xenia-project/xenia | Archival upstream |
| `Xenia-Edge/` | has207/xenia-edge | Edge fork; port queue |
| `XeniOS/` | xenios-jp/XeniOS | Apple-focused fork |

Sync scripts (workspace root): `scripts/sync-upstreams.ps1`, `scripts/analyze-upstream-updates.ps1`.

## `xenia-phoenix-src/` layout

```
xenia-phoenix-src/
  src/xenia/          app, cpu, gpu, kernel, apu, patcher, vfs
  tools/              tier0, phoenixctl, phoenix-mcp, vmx128_fuzz, gpu_replay_ci
  docs/               TIER0_*, patch_debt_dashboard, port status
  tests/              gpu_traces/, xma2_packets/
  game-patches/       per-title patches (in-tree or sibling clone)
  telemetry/          gitignored smoke captures
  .github/workflows/  tier0-differential, tier0-windows, ports
```

### High-signal `src/xenia/` areas

| Subdir | Contents |
|--------|----------|
| `app/` | `xenia_canary.exe` entry |
| `cpu/` | PPC + JIT (x64, ARM64 backends) |
| `gpu/` | Trace recorder, D3D12/Vulkan backends |
| `kernel/` | xboxkrnl / XAM shims, `stub_trace` |
| `apu/` | XMA2, APU trace hooks |

### `tools/` (Phoenix Tier 0)

| Directory | Purpose |
|-----------|---------|
| `tier0/` | Bootstrap, `categorize_patches.py`, smoke capture scripts |
| `phoenixctl/` | CLI for launch, tail, triage, gates |
| `phoenix-mcp/` | Cursor MCP server |
| `vmx128_fuzz/` | VMX128 differential fuzzer |
| `gpu_replay_ci/` | Trace replay CI (`run.py`, validators) |

## Paths with spaces

Workspace example: `G:\The Xenia Project\`. Always quote paths in PowerShell:

```powershell
$XeniaRoot = "G:\The Xenia Project"
$src = "$XeniaRoot\Xenia-Phoenix\xenia-phoenix-src"
```

WSL: `/mnt/g/The Xenia Project/Xenia-Phoenix/xenia-phoenix-src`

## Deeper detail (agents)

- [../metacache/workspace/01_canonical_paths.md](../metacache/workspace/01_canonical_paths.md)
- [../metacache/dev/30_repo_layout.md](../metacache/dev/30_repo_layout.md)
- [../metacache/workspace/00_monorepo_pointers.md](../metacache/workspace/00_monorepo_pointers.md)
