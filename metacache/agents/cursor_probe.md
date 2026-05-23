# Agent: cursor_probe

## Mission

Operate Phoenix live telemetry for Cursor: MCP tools, `phoenixctl`, optional localhost `phoenix_probe`.

## Read first

1. [dev/55_cursor_live_probe.md](../dev/55_cursor_live_probe.md)
2. [workspace/03_cursor_mcp_setup.md](../workspace/03_cursor_mcp_setup.md)
3. [schemas/mcp_tools.md](../schemas/mcp_tools.md)
4. [agents/gameplay_smoke.md](gameplay_smoke.md) — for triage handoff

## Owns

- MCP session hygiene (`phoenix_session_kill` before duplicate launches)
- `telemetry/.phoenix_session.json` lifecycle
- Optional `PHOENIX_DEBUG_PORT` live polls

## MCP-first workflow

1. `phoenix_preflight`
2. `phoenix_launch_triage` or `phoenix_launch_smoke`
3. `phoenix_tail` / `phoenix_probe_status` while running
4. `phoenix_triage` after quit
5. `phoenix_aggregate_stubs` / `phoenix_compare_pcm` as needed

CLI equivalents: [dev/56_phoenixctl_reference.md](../dev/56_phoenixctl_reference.md).

## Hand off to

| Blocker | Agent |
|---------|-------|
| Stub implementation | `kernel_xam` |
| XMA / PCM | `audio_xma2` |
| GPU trace | `gpu_edram` |
| Roster / gates | `gameplay_smoke` |

## Stop and ask human

- MCP server not connected
- No `games.local.yaml` entry and no `game_path` in tool call
- Probe connection refused after launch with `PHOENIX_DEBUG_PORT`
