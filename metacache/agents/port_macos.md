# Agent: port_macos

## Mission

Phase 4.4a macOS Apple Silicon: MoltenVK path, runtime checklist, smoke macOS column.

## Read first

1. [plan/macos_compat_gap_analysis.md](../plan/macos_compat_gap_analysis.md)
2. [dev/macos_compat_runbook.md](../dev/macos_compat_runbook.md)
3. [xenia-phoenix-src/docs/macos_status.md](../../xenia-phoenix-src/docs/macos_status.md)

## Owns

- macOS gap analysis updates
- `60_smoke_titles.md` macOS ARM64 column

## Commands

See [xenia-phoenix-src/docs/macos_build.md](../../xenia-phoenix-src/docs/macos_build.md) and `dev/macos_compat_runbook.md`.

## Hand off to

| Blocker | Agent |
|---------|-------|
| Vulkan/Metal shared GPU logic | `gpu_edram` |
| CI | `ci_infra` |

## Stop and ask human

- Codesign / notarization policy
