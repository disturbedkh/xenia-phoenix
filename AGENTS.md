# Phoenix — Cursor agent rules

1. Read [`metacache/INDEX.md`](metacache/INDEX.md) or [`metacache/QUICKSTART.md`](metacache/QUICKSTART.md) at session start.
2. For gameplay telemetry in Cursor, enable the **phoenix** MCP server ([`metacache/workspace/03_cursor_mcp_setup.md`](metacache/workspace/03_cursor_mcp_setup.md)) and follow [`metacache/dev/55_cursor_live_probe.md`](metacache/dev/55_cursor_live_probe.md).
3. Pick a role from [`metacache/agents/registry.yaml`](metacache/agents/registry.yaml) and follow that agent card (`cursor_probe` for MCP workflows).
4. Build and edit code only in [`xenia-phoenix-src/`](xenia-phoenix-src/) — use [`metacache/dev/10_build_commands.md`](metacache/dev/10_build_commands.md) and [`metacache/dev/20_test_commands.md`](metacache/dev/20_test_commands.md) literally.
5. End session with [`metacache/meta/01_handoff.md`](metacache/meta/01_handoff.md).
6. Never commit `xenia-phoenix-src/telemetry/`, `metacache/cache/games.local.yaml`, game ISO paths, or credentials.

Edit tools only under `xenia-phoenix-src/tools/`.
