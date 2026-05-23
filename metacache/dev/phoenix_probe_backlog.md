# Phoenix probe backlog (Phase 4)

Implemented in v2: `/health` (version 2), `/snapshot`, `/events`, launch guard, `probe_live.ps1`, MCP + phoenixctl wrappers.

## Not yet implemented

- **SSE or WebSocket `/stream`**: 1–10 Hz push of probe events for long-running agent sessions.
- **Guest freeze snapshot hotkey**: guest PC, module+export, GPU queue depth (design TBD under `metacache/dev/`).
- **VFS event sampling**: wire `PhoenixProbeNotifyVfsResolveFail` from `DiscImageDevice::ResolvePath` (deprioritized unless pre-gfx failures).

## Verification

- `tools/tier0/probe_http_smoke.ps1` after building `xenia-app`.
- BF2: `phoenixctl launch triage` + `phoenixctl probe snapshot` at repro; F4 for in-game `.xtr`.
