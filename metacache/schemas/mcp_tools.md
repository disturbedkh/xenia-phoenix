# MCP tool contract (phoenix server)

Server: `tools/phoenix-mcp/phoenix_mcp/server.py` (stdio). All tools return **JSON text** in the tool result.

## Tools

### `phoenix_preflight`

| Param | Type | Default |
|-------|------|---------|
| `config` | string | `Release` |

**Returns:** `{ exit_code, output }`

### `phoenix_launch_smoke`

| Param | Type | Default |
|-------|------|---------|
| `title_id` | string | required |
| `game_path` | string | `""` → games.local.yaml |
| `duration_sec` | int | `300` |
| `config` | string | `Release` |

**Returns:** `{ exit_code, paths, game_basename, stdout?, stderr? }`

### `phoenix_launch_triage`

| Param | Type | Default |
|-------|------|---------|
| `title_id` | string | required |
| `game_path` | string | `""` |
| `fresh_session` | bool | `false` |
| `duration_sec` | int | `0` |

**Returns:** `{ exit_code, mode, pid?, session?, paths, game_basename }`

### `phoenix_triage`

| Param | Type |
|-------|------|
| `title_id` | string |

**Returns:** `{ exit_code, report }`

### `phoenix_tail`

| Param | Type | Default |
|-------|------|---------|
| `title_id` | string | required |
| `kind` | `stubs\|xma\|pcm\|crash` | `stubs` |
| `lines` | int | `20` |

**Returns:** `{ kind, path, lines[], total_lines?, exists }`

### `phoenix_paths`

**Returns:** path map (relative paths).

### `phoenix_session_status` | `phoenix_session_kill`

**Returns:** session object (see `telemetry/.phoenix_session.json`); kill returns `{ killed, pid }`.

### `phoenix_list_patches`

**Returns:** `{ exit_code, output }`

### `phoenix_aggregate_stubs`

**Returns:** `{ exit_code, data }` — stub summary JSON.

### `phoenix_compare_pcm`

| Param | Type |
|-------|------|
| `title_id` | string |
| `baseline_path` | string optional |

### `phoenix_gate_vmx128` | `phoenix_gate_gpu_replay`

**Returns:** `{ exit_code, output }`

### `phoenix_probe_status` | `phoenix_probe_health` | `phoenix_probe_cvars` | `phoenix_probe_snapshot` | `phoenix_probe_events`

| Param | Type | Default |
|-------|------|---------|
| `port` | int | `0` → env `PHOENIX_DEBUG_PORT` |
| `names` | string | *(cvars only)* comma-separated |
| `since_seq` | int | `0` *(events only)* |

**Returns:** `{ http_status, body }` (JSON parsed in `body` when applicable)

### `phoenix_repro_capture`

| Param | Type | Default |
|-------|------|---------|
| `title_id` | string | required |
| `game_path` | string | `""` |
| `launch` | bool | `false` |
| `post_only` | bool | `true` |
| `legacy_gfx` | bool | `false` |
| `poll_count` / `poll_sec` | int | `6` / `5` |
| `trace_path` | string | `""` (newest in telemetry gpu_trace dir) |

**Returns:** JSON from `gpu_repro_capture.ps1` (probe polls, log_scan, trace_validate, fix_bar hints).

### `phoenix_log_scan` | `phoenix_log_summarize` | `phoenix_trace_validate` | `phoenix_trace_dump`

| Tool | Params |
|------|--------|
| `phoenix_log_scan` | `title_id`, optional `telemetry_dir` |
| `phoenix_log_summarize` | `title_id`, optional `write_summary`, `visual_pass` / `visual_fail` — wraps `phoenixctl log summarize` |
| `phoenix_trace_validate` | `path` to `.xtr` |
| `phoenix_trace_dump` | `path` to `.xtr` (slow; may fail on incomplete traces) |

`phoenix_obs_tail` tails `telemetry/{tid}_events.jsonl` (optional `code`, `channel` filters).

## Path redaction

Success payloads use **relative** paths under `xenia-phoenix-src`. Game paths appear as `game_basename` only unless the user supplied `game_path` in the same call.
