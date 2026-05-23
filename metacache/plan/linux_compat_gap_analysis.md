# Linux compat gap analysis (Phase 2.0)

Last updated: **2026-05-23**. **Roadmap:** all Linux work is Phase **2.x** in [20_tier_roadmap.md](20_tier_roadmap.md) (not Phase 3/4).

## Build matrix (Phase 2.2–2.4)

| Config | Docker | CI | Tests |
|--------|--------|-----|-------|
| Release | Green + verify | Required | `xenia-cpu-tests` + smoke + AppImage |
| Debug | `-Config all` / CI | Required | Smoke only |
| Checked | `-Config all` / CI | Required | Smoke only |

## Automation inventory (Phase 2.3–2.4)

| Item | Status |
|------|--------|
| `tools/docker/linux-build.sh` | Done |
| `tools/docker/linux-verify.sh` | Done |
| `tools/docker/run-linux-build.ps1` `-Config all` | Done |
| `tools/docker/Dockerfile` | Done |
| `.gitattributes` LF guards | Done |
| CI doctor + cpu-tests + AppImage smoke | Done |

## Manual gates (Phase 2.5–2.7)

| Sub-phase | Item | Status |
|-----------|------|--------|
| 2.5 | [linux_runtime_checklist.md](../../xenia-phoenix-src/docs/linux_runtime_checklist.md) | Doc done; sign-off pending |
| 2.6 | [60_smoke_titles.md](60_smoke_titles.md) Linux column | Template + slots; user fills titles |
| 2.7 | Clean VM AppImage window | Manual |

## Open questions

- Binary name remains `xenia_canary` on Linux.
- Arch/Fedora: package table in runtime checklist; no CI runner yet.
