# phoenixctl reference

Run from `xenia-phoenix-src` after `pip install -e tools/phoenixctl`.

Global flag: `--json` for machine-readable stdout (may appear before or after the subcommand, e.g. `phoenixctl repro capture ... --json`).

## Commands

### `preflight [--config Release]`

Runs `tools/tier0/smoke_session_ready.ps1`. Exit **0** if build + scripts OK.

### `launch smoke --title-id ID [--game PATH] [--duration 300] [--config Release]`

Runs `run_smoke_capture.ps1`. Resolves `--game` from `metacache/cache/games.local.yaml` if omitted.

### `launch triage --title-id ID [--game PATH] [--fresh] [--duration 0] [--config Auto]`

Starts instrumented Xenia (GUI). Writes `telemetry/.phoenix_session.json` with PID.

- `--duration > 0`: delegated timed capture via `launch_bf2_triage.ps1`
- `PHOENIX_DEBUG_PORT=8765`: adds `--phoenix_debug_port=8765`
- BF2 / repro scripts also pass `--log_preset=develop` and `--obs_events_log=telemetry/{tid}_events.jsonl` (see [observability.md](observability.md))

### `triage --title-id ID`

Runs `triage_gameplay_capture.ps1`. Exit **0** = all gates pass.

### `tail {stubs|xma|pcm|crash} --title-id ID [--lines 20]`

Prints last N lines of the matching telemetry file.

### `paths --title-id ID`

JSON map of telemetry paths (relative to repo root).

### `session status` | `session kill`

Read or stop the session recorded in `telemetry/.phoenix_session.json`.

### `patches --title-id ID`

Runs `list_smoke_patches.py`.

### `aggregate-stubs --title-id ID`

Runs `aggregate_stub_hits.py` → `telemetry/{tid}_stub_summary.json`.

### `compare-pcm --title-id ID [--baseline PATH]`

Runs `compare_pcm_hash_log.py` vs baseline (default `telemetry/{tid}_pcm_baseline.jsonl`).

### `gate vmx128 [--iters 10000] [--seed 42]`

### `gate gpu-replay`

### `gate patch-debt`

### `repro capture --title-id ID [--game PATH] [--launch] [--post-only] [--legacy-gfx] [--poll-count N]`

Runs [tools/tier0/gpu_repro_capture.ps1](../../xenia-phoenix-src/tools/tier0/gpu_repro_capture.ps1): optional triage launch, probe poll, `log scan`, `trace validate`, **`log summarize --write-summary`**, fix-bar JSON. See [gpu_fix_bar.md](gpu_fix_bar.md).

### `probe status|health|cvars|snapshot|events [--port N] [--since-seq N]`

HTTP client for in-emulator probe (v3 `/health`; `/snapshot` includes GPU + **obs invariant counters** + launch cvars). Default port from `PHOENIX_DEBUG_PORT` (8765).

### `log scan --title-id ID`

Counts `Invalid upload range for GPU` in crash log and runs `triage_crash_log.py` buckets.

### `log summarize --title-id ID [--telemetry-dir telemetry] [--write-summary] [--visual-pass|--visual-fail]`

Reads `telemetry/{tid}_events.jsonl`; prints top codes, domains, time range, `.xtr` list. With `--write-summary`, writes `telemetry/{tid}_obs_summary.json`. For `454107DB`, includes `classification` block mapping invariant codes → `U-GPU-001` / `G-454107DB-003`. See [observability.md](observability.md).

### `trace validate PATH` | `trace dump PATH`

Wraps `tools/gpu_replay_ci/validate_traces.py` and `xenia-gpu-d3d12-trace-dump` (headless; empty traces exit 5).

## Exit codes

| Code | Meaning |
|------|---------|
| 0 | Success / gates pass |
| 1 | Failure / triage fail / probe unreachable |
| 2 | Missing input files (e.g. stub log for aggregate) |

## Environment

| Variable | Purpose |
|----------|---------|
| `PHOENIX_SRC` | Override xenia-phoenix-src root |
| `PHOENIX_METACACHE` | Override metacache path |
| `PHOENIX_DEBUG_PORT` | Enable probe + pass to launch |
