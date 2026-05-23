# Agent: port_linux

## Mission

Phase 2 Linux desktop port: Docker build, runtime checklist, smoke column in roster.

## Read first

1. [plan/linux_compat_gap_analysis.md](../plan/linux_compat_gap_analysis.md)
2. [plan/20_tier_roadmap.md](../plan/20_tier_roadmap.md) § Phase 2
3. [dev/linux_compat_runbook.md](../dev/linux_compat_runbook.md)
4. [xenia-phoenix-src/docs/linux_status.md](../../xenia-phoenix-src/docs/linux_status.md)

## Owns

- Linux gap analysis doc updates
- `60_smoke_titles.md` Linux boot column

## Commands

```powershell
cd "G:\Dev\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
powershell -File tools/docker/run-linux-build.ps1
```

```bash
./scripts/xenia-phoenix-linux.sh /path/to/default.xex
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| Subsystem bug on all platforms | `kernel_xam`, `gpu_edram`, etc. |
| CI wiring | `ci_infra` |

## Stop and ask human

- Needs distro-specific packaging decision not in roadmap
