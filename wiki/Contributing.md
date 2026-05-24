# Contributing to Phoenix

Phoenix welcomes fixes that **improve native accuracy** and retire patch debt — not patches that hide emulator bugs.

## Before you start

1. Read [.github/CONTRIBUTING.md](../xenia-phoenix-src/.github/CONTRIBUTING.md) (style, content rules, no illegal activity on the issue tracker).
2. Read [Patch-Debt.md](Patch-Debt.md) — avoid new category-C/D patches when a proper fix is feasible.
3. Read [Tier-Model.md](Tier-Model.md) — know which tier your change serves.

## Code style

- Follow [docs/style_guide.md](../xenia-phoenix-src/docs/style_guide.md).
- Run `xb format` before commit.
- Cite RE sources in comments where guest interfaces are involved.

## Branch and PR flow

1. Branch: `phoenix/<phase>/<short-name>` (e.g. `phoenix/1.3-kernel/stub-foo`).
2. Implement the **smallest** change in one subsystem.
3. Build and test per [Building.md](Building.md) and [Testing-and-CI.md](Testing-and-CI.md).
4. Open PR on [disturbedkh/xenia-phoenix](https://github.com/disturbedkh/xenia-phoenix) with [docs/TIER0_PR_TEMPLATE.md](../xenia-phoenix-src/docs/TIER0_PR_TEMPLATE.md).
5. Wait for `tier0-differential` (PR) or run `tier0-windows.yml` manually for full gate.

### PR labels (examples)

`[1.1-cpu]`, `[1.2-gpu]`, `[1.3-kernel]`, `[1.4-audio]`, `[debt]`, `[infra]`

### Local verification checklist

- `xenia-cpu-tests.exe`
- vmx128-fuzz 50k smoke (PR CI parity); 1M before major CPU merges
- `run_xma2_diff.ps1`
- `validate_traces.py` / GPU replay as applicable
- `categorize_patches.py` if patch triage changed
- `aggregate_stub_hits.py` on fixture JSONL

## Non-negotiables

| Rule | Why |
|------|-----|
| No piracy enablement | Research/education only |
| No kernel-shim regressions | Gate new behavior behind cvars |
| No GPU regressions on traced titles | Update golden hashes deliberately |
| Document deviations from Canary | Enables rebase and upstream contribution |

## Ethics and telemetry

- Do not commit ISO paths, keys, or `telemetry/` captures.
- MCP and probe must not leak full user paths (basename only).
- XDK material: paraphrase only — never paste NDA docs.

[../metacache/meta/04_legal_ethics.md](../metacache/meta/04_legal_ethics.md)

## Upstream

Generically good changes may be proposed to [xenia-canary](https://github.com/xenia-canary/xenia-canary). Phoenix is a fork, not a replacement project.

## Deeper detail (agents)

- PR runbook: [../metacache/dev/40_pr_runbook.md](../metacache/dev/40_pr_runbook.md)
- Workflow loop: [../metacache/dev/00_workflow.md](../metacache/dev/00_workflow.md)
- Agent protocol: [../metacache/meta/00_agent_protocol.md](../metacache/meta/00_agent_protocol.md)
