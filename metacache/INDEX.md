# Phoenix metacache — index

Persistent memory for Project Phoenix. **Read [QUICKSTART.md](QUICKSTART.md) first.**

## Decision tree

| You are… | Go to |
|----------|--------|
| New session, unknown task | [QUICKSTART.md](QUICKSTART.md) → [agents/orchestrator.md](agents/orchestrator.md) |
| Know agent role | [agents/registry.yaml](agents/registry.yaml) → `agents/<id>.md` |
| Know 360 subsystem | [re/INDEX.md](re/INDEX.md) |
| Game / translation finding | [findings/INDEX.md](findings/INDEX.md) |
| About to build or test | [dev/10_build_commands.md](dev/10_build_commands.md), [dev/20_test_commands.md](dev/20_test_commands.md) |
| Ending session | [meta/01_handoff.md](meta/01_handoff.md) |
| Cursor live probe / MCP | [dev/55_cursor_live_probe.md](dev/55_cursor_live_probe.md) |
| Logging / presets / JSONL events | [dev/observability.md](dev/observability.md) |
| Tier 1 gameplay work | [agents/gameplay_smoke.md](agents/gameplay_smoke.md) |

## Layout

```
metacache/
  INDEX.md, QUICKSTART.md
  agents/           registry + specialist cards
  meta/             protocol, handoff, metrics, ethics
  workspace/        monorepo pointers, paths, Windows env
  plan/             mission, roadmap, debt, gameplay gates
  re/               Xbox 360 RE notes (theory)
  findings/         per-game + universal translation findings (empirical)
  dev/              workflow, build, test, runbooks, cursor probe
  cache/            session_log, vmx128_runs, gameplay_status, games.local.yaml
  schemas/          telemetry JSONL, obs_event_v1, MCP tools, phoenix_probe HTTP
  links/            curated_urls.yaml
  tools/            validate_links.ps1
```

## Active agents (priority)

| Agent | Status | Entry |
|-------|--------|-------|
| `cursor_probe` | **active** | [55_cursor_live_probe](dev/55_cursor_live_probe.md) |
| `gameplay_smoke` | **active** | [61_tier1_gameplay_gate](plan/61_tier1_gameplay_gate.md) |
| `kernel_xam` | active | [40_kernel_xam](re/40_kernel_xam.md) |
| `patch_debt` | active | [30_debt_ledger](plan/30_debt_ledger.md) |
| `orchestrator` | active | [20_tier_roadmap](plan/20_tier_roadmap.md) |

Full list: [agents/registry.yaml](agents/registry.yaml).

## Plan (strategy)

| File | Purpose |
|------|---------|
| [plan/00_mission.md](plan/00_mission.md) | North star |
| [plan/20_tier_roadmap.md](plan/20_tier_roadmap.md) | Tier 0/1 + ports |
| [plan/30_debt_ledger.md](plan/30_debt_ledger.md) | Patch debt |
| [plan/60_smoke_titles.md](plan/60_smoke_titles.md) | Smoke roster |
| [plan/61_tier1_gameplay_gate.md](plan/61_tier1_gameplay_gate.md) | Gameplay checklist |
| [plan/62_gameplay_execution.md](plan/62_gameplay_execution.md) | Execution tracker |
| [plan/50_open_questions.md](plan/50_open_questions.md) | Parking lot |
| [plan/10_session_log.md](plan/10_session_log.md) | Recent highlights → [cache/session_log.md](cache/session_log.md) |

Port analyses: `plan/linux_compat_*`, `android_compat_*`, `arm64_compat_*`, `macos_compat_gap_analysis.md`.

## Findings (game → universal)

| Entry | Purpose |
|-------|---------|
| [findings/INDEX.md](findings/INDEX.md) | Promotion ladder, ID conventions |
| [findings/games/](findings/games/INDEX.md) | Per-title repro + `G-*` log |
| [findings/universal/](findings/universal/INDEX.md) | Cross-title `U-*` patterns |
| [dev/gpu_fix_bar.md](dev/gpu_fix_bar.md) | When a GPU fix counts as native |

## Cache (mutable)

| File | Purpose |
|------|---------|
| [cache/session_log.md](cache/session_log.md) | Append-only session history |
| [cache/vmx128_runs.md](cache/vmx128_runs.md) | Fuzz run log |
| [cache/gameplay_status.md](cache/gameplay_status.md) | Gameplay checkbox snapshot |

## Source-of-truth links

- **Human wiki:** [wiki/Home.md](../wiki/Home.md)
- **Build + develop:** [xenia-phoenix-src/](../xenia-phoenix-src/)
- Tier 0: `xenia-phoenix-src/docs/TIER0_README.md`
- Telemetry playbook: `xenia-phoenix-src/telemetry/README.md`
- Reference canary: `../../xenia-canary/` (read-only)
- Tools: `xenia-phoenix-src/tools/` — see [workspace/01_canonical_paths.md](workspace/01_canonical_paths.md)

## Meta / process

- [meta/00_agent_protocol.md](meta/00_agent_protocol.md)
- [meta/03_metrics.md](meta/03_metrics.md)
- [plan/40_meta_goals.md](plan/40_meta_goals.md) → index into `meta/`
