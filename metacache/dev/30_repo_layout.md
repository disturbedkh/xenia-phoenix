# Repo layout — xenia-phoenix-src

Canonical build tree: `Xenia-Phoenix/xenia-phoenix-src/`. This is the AI's mental map.

## Top level

```
xenia-phoenix-src/
  src/                    main C++ source
  third_party/            vendored deps (submodules)
  tools/                  tier0, phoenixctl, phoenix-mcp, vmx128_fuzz, gpu_replay_ci, …
  src/xenia/debug/        phoenix_probe HTTP (localhost), debug/ui
  game-patches/           per-title TOML patches (debt ledger source)
  docs/                   TIER0_*, port status, kernel_stub_inventory.json
  tests/                  gpu_traces/, xma2_packets/
  telemetry/              gitignored smoke captures
  .github/workflows/      tier0-differential, tier0-windows, vmx128-1m-weekly, ports
  android/                Phase 3 Android project
  CMakeLists.txt
```

## src/xenia/

| Subdir | What's in it |
|--------|--------------|
| `app/` | `xenia_canary.exe` entry |
| `apu/` | XMA2, APU trace hooks |
| `cpu/` | PPC + JIT (x64, a64 backends) |
| `cpu/testing/` | `guest_ppc_test_util`, guest PPC tests |
| `gpu/` | trace recorder, D3D12/Vulkan backends |
| `kernel/xboxkrnl/`, `kernel/xam/` | kernel shims |
| `kernel/util/` | `stub_trace.{h,cc}` |
| `patcher/` | per-game patch system |
| `vfs/` | STFS, SVOD, host FS |

## tools/ (Phoenix Tier 0)

```
tools/tier0/           bootstrap.ps1, categorize_patches.py, run_smoke_capture.ps1, …
tools/phoenixctl/      phoenixctl CLI (launch, tail, triage, gates)
tools/phoenix-mcp/     Cursor MCP server (stdio)
tools/vmx128_fuzz/     differential fuzzer
tools/gpu_replay_ci/   run.py, validate_traces.py, gen_phase12_fixture_xtr.py
tools/xma2_diff/       XMA2 fixture diff
tools/docker/          Linux / Android Docker builds
```

## docs/ (high signal)

- `TIER0_README.md`, `TIER0_BUILD.md`, `TIER0_PR_TEMPLATE.md`
- `patch_debt_dashboard.md`, `patch_debt_dashboard_data.json`
- `vmx128_fuzz_report.json`, `kernel_stub_inventory.json`
- `linux_status.md`, `android_status.md`, `arm64_status.md`, `macos_status.md`

## CI workflows

- `tier0-differential.yml` — PR gate (Orchestrator)
- `tier0-windows.yml` — manual full Tier 0
- `vmx128-1m-weekly.yml` — 1M opcode sweep
- Port workflows as listed under `.github/workflows/`

## Phoenix outside src

| Path | Role |
|------|------|
| `Xenia-Phoenix/metacache/` | Agent memory (this tree) |
| `../../xenia-canary-canary_experimental/` | Reference snapshot only |

Keep this file aligned with the tree after large refactors.
