# Cursor live probe — architecture

Phoenix gives Cursor agents **structured access** to Xenia runtime telemetry without improvising PowerShell paths.

## Layers

| Layer | Location | Use when |
|-------|----------|----------|
| **MCP** | `xenia-phoenix-src/tools/phoenix-mcp/` | Agent in Cursor — prefer MCP tools over raw shell |
| **phoenixctl** | `xenia-phoenix-src/tools/phoenixctl/` | Terminal, CI, scripting |
| **tier0 scripts** | `xenia-phoenix-src/tools/tier0/` | Called by phoenixctl; do not fork logic |
| **phoenix_probe** | `src/xenia/debug/phoenix_probe.*` | Sub-second status while game runs (`PHOENIX_DEBUG_PORT`) |
| **File telemetry** | `xenia-phoenix-src/telemetry/` | JSONL + logs (gitignored) |
| **Observability v2** | `src/xenia/base/obs/` | Presets, unified events, invariants — [observability.md](observability.md) |

## Typical workflow (gameplay triage)

1. `phoenix_preflight` or `phoenixctl preflight`
2. Register game path in `metacache/cache/games.local.yaml` (copy from `games.local.yaml.example`)
3. `phoenix_launch_triage` with `title_id` (+ optional `game_path`)
4. Play to crash; optionally `phoenix_tail` on `stubs` / `pcm`
5. `phoenix_triage` → read report + exit code
6. Hand off to `kernel_xam` / `audio_xma2` / `gpu_edram` per failures

## Near-live vs in-process

| Need | Mechanism |
|------|-----------|
| Stub hits as they happen | Tail `telemetry/{tid}_stubs.jsonl` (~per hit) |
| PCM drift (~1 Hz) | Tail `telemetry/{tid}_pcm.jsonl` |
| Live title + GPU counters | `phoenix_probe_snapshot` with `PHOENIX_DEBUG_PORT=8765` |
| Incremental events (upload errors, stubs) | `phoenix_probe_events` / `phoenixctl probe events` |
| Unified JSONL (stubs + GPU + invariants) | `telemetry/{tid}_events.jsonl` — develop preset |
| Post-session classification | `phoenixctl log summarize --write-summary` → `obs_summary.json` |
| Post-session gates | `phoenix_triage`, `phoenix_aggregate_stubs`, `repro capture --post-only` |
| Misconfigured launch guard | `phoenixctl launch triage` → `launch_guard` in JSON (readback/hid/telemetry paths) |

Tier0: `probe_http_smoke.ps1`, `probe_live.ps1 -TitleId <id>`, **`gpu_repro_capture.ps1`** / `phoenixctl repro capture`.

Snapshot `gpu` block includes EDRAM counters: `ownership_change_count`, `edram_transfer_count`, `host_depth_store_count`, `host_depth_transfer_mismatch_count`.

## Schemas

- JSONL columns: [`schemas/telemetry.md`](../schemas/telemetry.md) (incl. §8–9 obs events + summary)
- Obs event schema: [`schemas/obs_event_v1.json`](../schemas/obs_event_v1.json)
- MCP tools: [`schemas/mcp_tools.md`](../schemas/mcp_tools.md)
- HTTP probe: [`schemas/phoenix_probe_http.md`](../schemas/phoenix_probe_http.md)

## Setup

- MCP: [`workspace/03_cursor_mcp_setup.md`](../workspace/03_cursor_mcp_setup.md)
- CLI reference: [`dev/56_phoenixctl_reference.md`](56_phoenixctl_reference.md)
- Agent card: [`agents/cursor_probe.md`](../agents/cursor_probe.md)

## Security

- Probe binds **127.0.0.1 only**; default port **0** (off).
- Do not commit `games.local.yaml`, `telemetry/`, or ISO paths.
- See [`meta/04_legal_ethics.md`](../meta/04_legal_ethics.md).
