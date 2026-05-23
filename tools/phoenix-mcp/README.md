# phoenix-mcp

stdio MCP server exposing Phoenix telemetry tools to Cursor.

Setup: [`Xenia-Phoenix/metacache/workspace/03_cursor_mcp_setup.md`](../../../metacache/workspace/03_cursor_mcp_setup.md).

```powershell
pip install -e ../phoenixctl
pip install -e .
python -m phoenix_mcp.server
```
