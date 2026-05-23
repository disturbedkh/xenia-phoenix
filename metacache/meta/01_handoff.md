# Session handoff checklist

Run at end of every agent session.

## Always

1. Append **1–3 bullets** to [cache/session_log.md](../cache/session_log.md).
2. If milestone-level, add one line to [plan/10_session_log.md](../plan/10_session_log.md) highlights.
3. Update [cache/gameplay_status.md](../cache/gameplay_status.md) if gameplay checkboxes changed.

## When applicable

| Event | Update |
|-------|--------|
| Patch retired or reclassified | [plan/30_debt_ledger.md](../plan/30_debt_ledger.md) |
| New pitfall | [dev/60_known_pitfalls.md](../dev/60_known_pitfalls.md) |
| VMX128 fuzz / sweep | [cache/vmx128_runs.md](../cache/vmx128_runs.md) |
| MCP / phoenixctl session | Note tools used + triage exit code; `phoenix_session_kill` if left running |
| 360 RE finding (theory) | relevant `re/*.md` |
| Translation finding (game or pattern) | [findings/games/](../findings/games/) and/or [findings/universal/](../findings/universal/); cite `G-*` / `U-*` in session log |
| GPU fix claimed done | [dev/gpu_fix_bar.md](../dev/gpu_fix_bar.md) checklist |
| Open decision | [plan/50_open_questions.md](../plan/50_open_questions.md) |
| Roadmap phase change | [plan/20_tier_roadmap.md](../plan/20_tier_roadmap.md) |

## Do not commit

- `xenia-phoenix-src/telemetry/*.jsonl`
- Game ISO paths or credentials
- Large `.xtr` corpora (unless explicitly part of a PR)

## Staleness

Set `last_verified` in cache file front matter when you confirm metrics still hold.
