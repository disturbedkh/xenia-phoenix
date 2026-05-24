# Xenia Phoenix

**Xenia Phoenix** is an Xbox 360 emulator fork aimed at **Tier 1 — native, accurate 360 emulation**. Progress is measured with differential tests, GPU trace hashes, kernel stub logs, and patch-debt retirement — not by how many titles boot once on someone's PC.

Phoenix inherits the Xenia Canary codebase and deliberately diverges where accuracy requires it. Per-game patches are **technical debt to pay down**, not a permanent compatibility strategy.

**Repository:** [github.com/disturbedkh/xenia-phoenix](https://github.com/disturbedkh/xenia-phoenix)

## Current status

| Phase | Status |
|-------|--------|
| **Tier 0** — PC-only differential CI (VMX128 fuzz, GPU replay, patch categorizer, stub JSONL) | **Done** on x64 Windows |
| **Tier 1-PC** — differential gates without gameplay | **Complete** (2026-05-16) |
| **Tier 1-Gameplay** — smoke captures, C/D patch retirement, retail traces, new-title boot | **In progress** |

Details: [wiki/Home.md](wiki/Home.md) · [wiki/Tier-Model.md](wiki/Tier-Model.md) · [metacache/plan/61_tier1_gameplay_gate.md](metacache/plan/61_tier1_gameplay_gate.md)

## North star

Project Phoenix exists to move from "many games boot with hand-tuned per-title patches" to **bit-accurate emulation that does not need per-game patches for the average title**.

- **Patches are debt**, not features — each `game-patches/*.patch.toml` entry papers over an emulator gap.
- **Tier 1 is reachable from a PC alone** — VMX128, EDRAM, kernel shims, and XMA2 are testable without console hardware.
- **The compatibility list is a lagging indicator** — Phoenix tracks trace hashes, stub JSONL, and patch categories instead.

Full mission: [metacache/plan/00_mission.md](metacache/plan/00_mission.md)

## Repository layout

| Path | Role |
|------|------|
| [`xenia-phoenix-src/`](xenia-phoenix-src/) | **Canonical build tree** — C++, CMake, CI, tools ([disturbedkh/xenia-phoenix](https://github.com/disturbedkh/xenia-phoenix), `canary_experimental`) |
| [`metacache/`](metacache/) | Agent memory — strategy, RE notes, build/test runbooks (`metacache` branch) |
| [`wiki/`](wiki/Home.md) | Human-facing documentation |
| [`game-patches/`](game-patches/) | Per-title patch tree (sibling clone; patch debt source) |
| `../xenia-canary/` | Reference Canary snapshot (read-only) |
| `../xenia-master/` | Archival upstream Xenia |
| `../Xenia-Edge/` | Edge fork (port queue reference) |
| `../XeniOS/` | Apple-focused fork reference |

Monorepo map: [wiki/Project-Structure.md](wiki/Project-Structure.md) · [metacache/workspace/00_monorepo_pointers.md](metacache/workspace/00_monorepo_pointers.md)

## Tier model

| Tier | Description | Phoenix policy |
|------|-------------|----------------|
| **0** | PC-only differential testing, internal-consistency CI, build hygiene | Always green. CI gate. |
| **1** | True/native 360 emulation: VMX128, EDRAM/Xenos, XMA2, kernel coverage | **Primary target.** Every PR moves toward Tier 1. |
| **2** | Per-game compatibility patches (`game-patches`) | Tracked in patch debt; goal is shrink, not grow. |
| **A** | HDMI capture-card ground truth (no console mods) | Optional uplift; deferred until Tier 0/1 saturate. |
| **B** | Modded-console / homebrew capture tools | Out of scope until A is exhausted. |
| **C** | Devkit / PIX-for-Xbox-360 | Effectively unobtainable in 2026. |

**Definition of done for Tier 1:** [wiki/Tier-Model.md](wiki/Tier-Model.md) · [metacache/plan/00_mission.md](metacache/plan/00_mission.md)

## Quick start (Windows)

**Prerequisites:** Visual Studio 2022 (Desktop C++), CMake, Ninja, Vulkan SDK, Python 3, Git.

**Critical:** builds must run inside `vcvars64` or MSVC headers will be missing.

Set your tree root (adjust drive/path as needed):

```powershell
$XeniaRoot = "G:\The Xenia Project"   # workspace root
$src       = "$XeniaRoot\Xenia-Phoenix\xenia-phoenix-src"
$vcvars    = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

Incremental build:

```powershell
cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py build"
```

Full bootstrap, flags, and port builds: [wiki/Building.md](wiki/Building.md) · [metacache/dev/10_build_commands.md](metacache/dev/10_build_commands.md)

## Documentation

| Audience | Start here |
|----------|------------|
| **Everyone** | [wiki/Home.md](wiki/Home.md) |
| **Developers** | [wiki/Building.md](wiki/Building.md) · [wiki/Testing-and-CI.md](wiki/Testing-and-CI.md) |
| **Contributors** | [wiki/Contributing.md](wiki/Contributing.md) · [wiki/Patch-Debt.md](wiki/Patch-Debt.md) |
| **Tier 0 / CI** | [xenia-phoenix-src/docs/TIER0_README.md](xenia-phoenix-src/docs/TIER0_README.md) |
| **Cursor / agents** | [metacache/QUICKSTART.md](metacache/QUICKSTART.md) · [AGENTS.md](AGENTS.md) |

## Contributing

- Prefer **fixing the emulator** over adding `game-patches` entries (categories C and D).
- Kernel and GPU changes must respect existing shim contracts and trace hashes — see [wiki/Contributing.md](wiki/Contributing.md).
- Legal / ethics: [metacache/meta/04_legal_ethics.md](metacache/meta/04_legal_ethics.md)

## Disclaimer

Phoenix is research and educational. It is **not** for enabling piracy. Tooling that requires asset dumps assumes you legally own the relevant disc or console.
