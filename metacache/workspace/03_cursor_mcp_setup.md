# Cursor MCP setup (Phoenix)

## 1. Install Python tools

```powershell
cd "G:\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
pip install -e tools/phoenixctl
pip install -e tools/phoenix-mcp
```

Requires Python 3.10+.

## 2. Register MCP server

Copy [`../../.cursor/mcp.json.example`](../../.cursor/mcp.json.example) to **one of**:

- Workspace: `Xenia-Phoenix/.cursor/mcp.json` (recommended)
- User global MCP settings in Cursor

Adjust `PHOENIX_SRC` if your workspace root differs:

```json
"env": {
  "PHOENIX_SRC": "${workspaceFolder}/Xenia-Phoenix/xenia-phoenix-src"
}
```

## 3. Verify in Cursor

1. Open **Cursor Settings → MCP** — `phoenix` server should show connected.
2. In Agent chat, tools `phoenix_preflight`, `phoenix_paths`, etc. should appear.
3. Run `phoenix_preflight` (needs a built `xenia_canary.exe` for full pass).

## 4. Game paths (gitignored)

```powershell
copy ..\metacache\cache\games.local.yaml.example ..\metacache\cache\games.local.yaml
# Edit title_id → path entries
```

Then: `phoenix_launch_triage` with `title_id` only.

## 5. Optional live probe

```powershell
$env:PHOENIX_DEBUG_PORT = "8765"
```

Launch triage via MCP or phoenixctl; poll with `phoenix_probe_status`.

## Troubleshooting

| Issue | Fix |
|-------|-----|
| MCP server won't start | `python -m phoenix_mcp.server` manually; check `pip install mcp` |
| `Cannot find xenia-phoenix-src` | Set `PHOENIX_SRC` in mcp.json `env` |
| Tools missing in chat | Reload window; confirm MCP enabled for Agent mode |
| Probe connection refused | Emulator not running or `phoenix_debug_port=0` |

See [`dev/55_cursor_live_probe.md`](../dev/55_cursor_live_probe.md).
