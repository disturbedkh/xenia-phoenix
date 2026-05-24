# Xenia Phoenix Wiki

Human-facing documentation for [Project Phoenix](../README.md). Agents use [metacache](../metacache/QUICKSTART.md) instead.

## What is Phoenix?

**Xenia Phoenix** is an Xbox 360 emulator fork that targets **Tier 1 — native, accurate 360 emulation**. It inherits the Xenia Canary codebase and pays down per-game patch debt rather than treating patches as permanent compatibility features.

Progress is measured with:

- VMX128 fuzz (JIT vs reference)
- GPU trace replay (RTV vs ROV hash parity)
- Kernel stub-hit JSONL on smoke titles
- Patch-debt categorization (retire categories C and D)

## Current status (2026-05)

| Phase | Status |
|-------|--------|
| Tier 0 — differential CI | **Done** |
| Tier 1-PC — gates without gameplay | **Complete** |
| Tier 1-Gameplay — smoke, patch retirement, retail traces | **In progress** |

Roadmap detail: [Tier-Model.md](Tier-Model.md) · [../metacache/plan/20_tier_roadmap.md](../metacache/plan/20_tier_roadmap.md)

## Wiki pages

| Page | Description |
|------|-------------|
| [Project-Structure.md](Project-Structure.md) | Monorepo layout, remotes, paths |
| [Tier-Model.md](Tier-Model.md) | Tiers 0–2, A–C, definition of done |
| [Building.md](Building.md) | Windows build, bootstrap, ports |
| [Testing-and-CI.md](Testing-and-CI.md) | vmx128-fuzz, GPU replay, workflows |
| [Patch-Debt.md](Patch-Debt.md) | Categories A–D, retirement workflow |
| [Tools.md](Tools.md) | phoenixctl, phoenix-mcp, tier0 |
| [Contributing.md](Contributing.md) | PR flow, ethics, labels |
| [FAQ.md](FAQ.md) | Phoenix vs Canary, compat list, patches |

## Other documentation

| Resource | Path |
|----------|------|
| Project README | [../README.md](../README.md) |
| Tier 0 in-tree | [../xenia-phoenix-src/docs/TIER0_README.md](../xenia-phoenix-src/docs/TIER0_README.md) |
| Agent quickstart | [../metacache/QUICKSTART.md](../metacache/QUICKSTART.md) |
| Telemetry playbook | [../xenia-phoenix-src/telemetry/README.md](../xenia-phoenix-src/telemetry/README.md) (local, gitignored captures) |

## Maintenance

When mission or tier phase changes, update **in order:**

1. [../metacache/plan/00_mission.md](../metacache/plan/00_mission.md) / [20_tier_roadmap.md](../metacache/plan/20_tier_roadmap.md)
2. This page + [Tier-Model.md](Tier-Model.md) status blurb
3. [../README.md](../README.md) status table

## Deeper detail (agents)

- Mission: [../metacache/plan/00_mission.md](../metacache/plan/00_mission.md)
- Roadmap: [../metacache/plan/20_tier_roadmap.md](../metacache/plan/20_tier_roadmap.md)
- Full index: [../metacache/INDEX.md](../metacache/INDEX.md)
