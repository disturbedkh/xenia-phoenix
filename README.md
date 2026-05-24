# Xenia Phoenix — Xbox 360 Emulator (build tree)

This directory is the **canonical C++ build tree** for [Xenia Phoenix](../README.md): a fork focused on native-accuracy Xbox 360 emulation and retiring per-game patch debt.

**Human docs:** [../wiki/Home.md](../wiki/Home.md) · **Project README:** [../README.md](../README.md)

**Repository:** [github.com/disturbedkh/xenia-phoenix](https://github.com/disturbedkh/xenia-phoenix) (`canary_experimental` branch)

## Relationship to Xenia Canary

Phoenix is forked from [xenia-canary/xenia-canary](https://github.com/xenia-canary/xenia-canary) (`canary_experimental`). It keeps Canary's broad platform coverage while prioritizing **measurable accuracy** (VMX128 differential tests, GPU trace replay, kernel stub tracing, patch categorization) over boot-and-patch compatibility.

- **Upstream remote:** `upstream` → xenia-canary (rebase / merge base)
- **Origin remote:** `origin` → disturbedkh/xenia-phoenix (Phoenix work)
- Generic emulator background (options, community): [Canary wiki](https://github.com/xenia-canary/xenia-canary/wiki) — reference only; Phoenix workflow docs live in [../wiki/](../wiki/)

## Status

| Area | State |
|------|--------|
| Tier 0 differential CI | Green on x64 Windows — see [docs/TIER0_README.md](docs/TIER0_README.md) |
| Tier 1-PC | Complete (2026-05-16) |
| Tier 1-Gameplay | In progress — [../wiki/Tier-Model.md](../wiki/Tier-Model.md) |

CI workflows: `tier0-differential.yml` (PR), `tier0-windows.yml` (manual), `vmx128-1m-weekly.yml`. See [../wiki/Testing-and-CI.md](../wiki/Testing-and-CI.md).

## Quick links

| Topic | Document |
|-------|----------|
| **Tier 0 pipeline** | [docs/TIER0_README.md](docs/TIER0_README.md) |
| **Build (Phoenix)** | [../wiki/Building.md](../wiki/Building.md) · [docs/TIER0_BUILD.md](docs/TIER0_BUILD.md) |
| **Build (general)** | [docs/building.md](docs/building.md) |
| **Style** | [docs/style_guide.md](docs/style_guide.md) |
| **Patch debt** | [docs/patch_debt_dashboard.md](docs/patch_debt_dashboard.md) · [../wiki/Patch-Debt.md](../wiki/Patch-Debt.md) |
| **Contributing** | [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) · [../wiki/Contributing.md](../wiki/Contributing.md) |

## Phoenix tools

| Tool | Purpose |
|------|---------|
| [tools/tier0/](tools/tier0/) | Bootstrap, patch categorizer, smoke capture, VMX128/GPU scripts |
| [tools/phoenixctl/](tools/phoenixctl/) | CLI — launch, tail telemetry, triage, Tier 0 gates |
| [tools/phoenix-mcp/](tools/phoenix-mcp/) | Cursor MCP server for live probe workflows |
| [tools/vmx128_fuzz/](tools/vmx128_fuzz/) | VMX128 differential fuzzer |
| [tools/gpu_replay_ci/](tools/gpu_replay_ci/) | GPU trace replay CI |

## Building

Phoenix Windows builds **must** run inside MSVC `vcvars64`. See [../wiki/Building.md](../wiki/Building.md) for the canonical recipe.

```powershell
# Example — set $src to this directory
$vcvars = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py build"
```

## Contributing

Read [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) for style and content rules.

**Phoenix-specific:**

- Do not add category-C/D patches when an emulator fix is possible — see [../wiki/Patch-Debt.md](../wiki/Patch-Debt.md).
- PRs should pass Tier 0 gates; use [docs/TIER0_PR_TEMPLATE.md](docs/TIER0_PR_TEMPLATE.md) for descriptions.
- Run `xb format` (or equivalent) before commit.

## Disclaimer

The goal of this project is to experiment, research, and educate on emulation of modern devices and operating systems. **It is not for enabling illegal activity.** All information is obtained via reverse engineering of legally purchased devices and games and information made public on the internet.
