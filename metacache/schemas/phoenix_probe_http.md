# phoenix_probe HTTP API (v3 health / v2 snapshot)

In-emulator localhost JSON API. **Off by default** (`--phoenix_debug_port=0`).

Bind: `127.0.0.1` only.

## Enable

```powershell
$env:PHOENIX_DEBUG_PORT = "8765"
phoenixctl launch triage --title-id 454107DB --game "D:\path\to\game.iso"
# or
.\tools\tier0\launch_bf2_triage.ps1 -GamePath "..."  # reads env automatically
```

Or CLI flag: `--phoenix_debug_port=8765`

## Endpoints

### `GET /health`

```json
{ "ok": true, "version": "3", "obs_schema": 1 }
```

(`version` **2** on older builds without observability v2.)

### `GET /status`

```json
{
  "title_id": "454107db",
  "uptime_ms": 120000,
  "stub_hit_count": 42,
  "last_stub_module": "xam",
  "last_stub_export": "XamShowMessageBoxUI",
  "last_pcm_hash": "abc123...",
  "xma_divergence_count": 0
}
```

Fields may be omitted when unset.

### `GET /cvars?names=kernel_stub_hit_log,apu_pcm_hash_log`

```json
{
  "cvars": {
    "kernel_stub_hit_log": "telemetry/454107db_stubs.jsonl",
    "apu_pcm_hash_log": "telemetry/454107db_pcm.jsonl"
  }
}
```

Unknown names are omitted. Read-only in v1.

### `GET /snapshot`

Unified live state (status + GPU counters + launch cvars):

```json
{
  "title_id": "454107db",
  "uptime_ms": 120000,
  "stub_hit_count": 0,
  "xma_divergence_count": 0,
  "gpu": {
    "upload_range_error_count": 42,
    "pipeline_skip_count": 0,
    "present_count": 3600,
    "ownership_change_count": 120,
    "edram_transfer_count": 120,
    "host_depth_store_count": 45,
    "host_depth_transfer_mismatch_count": 8,
    "obs_depth_host_sidecar_stale_count": 0,
    "obs_host_depth_transfer_mismatch_count": 0,
    "obs_gpu_upload_range_error_count": 0,
    "readback_resolve": "fast"
  },
  "launch": {
    "hid": "xinput",
    "trace_gpu_prefix": "telemetry/454107db_gpu_trace",
    "kernel_stub_hit_log": "telemetry/454107db_stubs.jsonl",
    "apu_pcm_hash_log": "telemetry/454107db_pcm.jsonl"
  },
  "event_seq": 128
}
```

### `GET /events?since_seq=0`

```json
{
  "events": [
    {"seq": 1, "ts_ms": 5000, "kind": "upload_range_error", "detail": "invalid_gpu_upload_range"}
  ],
  "latest_seq": 1
}
```

Kinds: `stub_hit`, `upload_range_error`, `vfs_resolve_fail`, `pipeline_skip`, `ownership_change`, `host_depth_store`.

Ring events are also bridged to **`obs_events_log`** (unified schema) when develop/forensic preset is active.

## Observability counters (`gpu` block)

| Field | Maps to invariant / signal |
|-------|----------------------------|
| `obs_depth_host_sidecar_stale_count` | `DepthHostSidecarStale` |
| `obs_host_depth_transfer_mismatch_count` | `HostDepthTransferMismatch` |
| `obs_gpu_upload_range_error_count` | `GpuUploadRangeError` |

Prefer **`phoenixctl log summarize`** for post-session classification; probe counters are live session hints.

## Errors

| HTTP | Body |
|------|------|
| 404 | `{ "error": "not_found" }` |
| 400 | `{ "error": "bad_request" }` |

## Client

```powershell
phoenixctl probe status
phoenixctl probe health --port 8765
phoenixctl probe snapshot --port 8765
phoenixctl probe events --since-seq 0
```

Smoke: `tools/tier0/probe_http_smoke.ps1`

MCP: `phoenix_probe_status`, `phoenix_probe_health`, `phoenix_probe_snapshot`, `phoenix_probe_events`
