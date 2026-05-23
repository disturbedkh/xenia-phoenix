# Agent: gameplay_smoke

## Mission

Finish Tier 1-Gameplay: smoke roster, 5-minute captures, empty stub/XMA logs, retail GPU trace.

## Read first

1. [plan/61_tier1_gameplay_gate.md](../plan/61_tier1_gameplay_gate.md)
2. [plan/62_gameplay_execution.md](../plan/62_gameplay_execution.md)
3. [cache/gameplay_status.md](../cache/gameplay_status.md)
4. [plan/60_smoke_titles.md](../plan/60_smoke_titles.md)
5. [xenia-phoenix-src/telemetry/README.md](../../xenia-phoenix-src/telemetry/README.md)

## Owns

- `plan/60_smoke_titles.md` columns (with user title IDs)
- `cache/gameplay_status.md`
- Per-title telemetry under `xenia-phoenix-src/telemetry/` (gitignored)

## Commands

**Prefer MCP** (`phoenix_launch_smoke`, `phoenix_triage`) or **phoenixctl** — see [dev/55_cursor_live_probe.md](../dev/55_cursor_live_probe.md).

```powershell
cd "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
phoenixctl launch smoke --title-id 4D5307D1 --game "path\to\default.xex"
phoenixctl triage --title-id 4D5307D1
```

Legacy scripts (called by phoenixctl):

```powershell
python tools/tier0/list_smoke_patches.py --title-id 4D5307D1
.\tools\tier0\run_smoke_capture.ps1 -TitleId 4D5307D1 -GamePath "path\to\default.xex"
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| Stub export needs implementation | `kernel_xam` |
| XMA / PCM divergence | `audio_xma2` |
| GPU trace replay failure | `gpu_edram` |
| Patch removal after fix | `patch_debt` |

## Stop and ask human

- No legally owned `default.xex` / ISO path for a roster title
- Capture requires >5 min interactive play you cannot script
