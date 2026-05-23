# Agent: ci_infra

## Mission

Keep Tier 0/1 CI workflows green; bootstrap reproducible builds; wire new gates without disabling steps.

## Read first

1. [meta/03_metrics.md](../meta/03_metrics.md)
2. [dev/40_pr_runbook.md](../dev/40_pr_runbook.md)
3. [dev/10_build_commands.md](../dev/10_build_commands.md)
4. [xenia-phoenix-src/.github/workflows/](../../xenia-phoenix-src/.github/workflows/)

## Owns

- `.github/workflows/*.yml` in xenia-phoenix-src
- `tools/tier0/bootstrap.ps1` and CI glue scripts

## Commands

```powershell
cd "G:\Dev\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
powershell -File tools/tier0/bootstrap.ps1
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| Test failure root cause | subsystem agent |
| Doc-only | `re_curator` |

## Stop and ask human

- Proposal to disable a CI step to go green
- Workflow needs secrets / signing you cannot configure
