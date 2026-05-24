# FAQ

Phoenix-specific questions. For generic Xenia options and community support, see the [Canary wiki](https://github.com/xenia-canary/xenia-canary/wiki) as reference only.

## What is the difference between Phoenix and Xenia Canary?

**Canary** optimizes for broad compatibility — many titles boot with per-game patches and community testing.

**Phoenix** optimizes for **native accuracy** — differential tests, GPU traces, kernel stub logs, and **retiring** patch debt (categories C/D). Phoenix forks Canary's `canary_experimental` branch and tracks intentional deviations in metacache.

## Should I use the Canary game compatibility list?

It remains useful as a **lagging indicator** of boot status, but Phoenix does **not** treat compat-list green as mission success. Progress is measured by Tier 0/1 gates, stub JSONL drain, and patch categories.

Contribute compat updates to Canary if you like; Phoenix gameplay work uses the [smoke roster](../metacache/plan/60_smoke_titles.md) and [Tier 1 gameplay gate](../metacache/plan/61_tier1_gameplay_gate.md).

## Where do per-game patches live?

- In-tree: `xenia-phoenix-src/game-patches/`
- Sibling clone: `Xenia-Phoenix/game-patches/` (some setups)

Patches are **debt**, not features. See [Patch-Debt.md](Patch-Debt.md).

## What is Tier 1-Gameplay?

After **Tier 1-PC** closed all differential gates without playing retail titles, **Tier 1-Gameplay** finishes Tier 1 with real smoke captures, C/D patch retirement, retail GPU traces (where legal), and proving a new title boots without a new patch.

Status: **in progress**. See [Tier-Model.md](Tier-Model.md).

## How do I build Phoenix?

Windows: MSVC `vcvars64` + `python xenia-build.py build`. See [Building.md](Building.md).

## Do I need an Xbox 360?

**No** for most Tier 1 work — VMX128, EDRAM, kernel shims, and XMA2 are testable on PC. Optional Tier A/B use capture hardware later.

## Can I run games I downloaded?

Phoenix does not enable piracy. You must legally own discs/content you run. Do not commit ISO paths or dumps to the repo.

## Where is agent / Cursor documentation?

- Agents: [../metacache/QUICKSTART.md](../metacache/QUICKSTART.md) and [../AGENTS.md](../AGENTS.md)
- MCP setup: [../metacache/workspace/03_cursor_mcp_setup.md](../metacache/workspace/03_cursor_mcp_setup.md)
- Human wiki: [Home.md](Home.md)

## How do I report a bug?

Use the Phoenix fork issue tracker on GitHub when available; include repro commands, Tier 0 artifacts if relevant, and **no** illegal content. For Canary-only boot issues, upstream may be more appropriate.

## Deeper detail (agents)

- Open questions: [../metacache/plan/50_open_questions.md](../metacache/plan/50_open_questions.md)
- Known pitfalls: [../metacache/dev/60_known_pitfalls.md](../metacache/dev/60_known_pitfalls.md)
