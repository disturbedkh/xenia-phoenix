# Smoke telemetry (Tier 1 gameplay)

Local-only capture output from [run_smoke_capture.ps1](../tools/tier0/run_smoke_capture.ps1) and [launch_bf2_triage.ps1](../tools/tier0/launch_bf2_triage.ps1). Files here are **gitignored**.

**Cursor agents:** use `phoenixctl` or the **phoenix** MCP server — see [metacache/dev/55_cursor_live_probe.md](../../metacache/dev/55_cursor_live_probe.md). Session PID: `.phoenix_session.json`.

**Standing still in-game:** set `$env:PHOENIX_DEBUG_PORT = "8765"`, launch triage, then run `.\tools\tier0\probe_bf2_live.ps1` (or `phoenixctl probe status` / `phoenixctl tail pcm`).

## Layout (per title)

| File | Source |
|------|--------|
| `{title_id}_stubs.jsonl` | `--kernel_stub_hit_log` |
| `{title_id}_xma.jsonl` | `--apu_xma_divergence_log` |
| `{title_id}_pcm.jsonl` | `--apu_pcm_hash_log` |
| `{title_id}_crash.log` | `--log_file` (instrumented triage launch) |
| `{title_id}_gpu_trace/` | `--trace_gpu_prefix` |
| `{title_id}_triage_report.txt` | `triage_gameplay_capture.ps1` output |
| `{title_id}_stub_summary.json` | `aggregate_stub_hits.py` |
| `{title_id}_xma_summary.json` | `aggregate_xma_divergences.py` |
| `{title_id}_pcm_baseline.jsonl` | `compare_pcm_hash_log.py --update-baseline` |

## Observability v2 artifacts

When launched with `--log_preset=develop` (default on triage/smoke scripts):

| File | Source |
|------|--------|
| `{title_id}_events.jsonl` | `--obs_events_log` unified host events (invariants, probes) |
| `{title_id}_obs_summary.json` | `phoenixctl log summarize --write-summary` |
| `{title_id}_gpu_trace/*.xtr.obs.json` | Trace sidecar: `obs_event_seq` window per F4 capture |

Presets: `play` (minimal), `support`, `homebrew`, `develop` (triage default), `forensic`
(writes `telemetry/{tid}_crash_bundle.zip` on shutdown).

```powershell
phoenixctl log summarize --title-id 454107DB
phoenixctl obs tail --title-id 454107DB --code DepthHostSidecarStale
phoenixctl trace explain telemetry/454107db_gpu_trace/frame_0001.xtr
```

MCP: `phoenix_log_summarize`, `phoenix_obs_tail`. Schema: `metacache/schemas/obs_event_v1.json`.

`title_id` is 8 hex digits without `0x` (e.g. `4d5307d1`).

## Roster capture (5 titles)

1. Copy `Xenia-Phoenix/metacache/dev/smoke_roster_local.example.toml` → `telemetry/smoke_roster_local.toml`
2. Set each `game_path` to your `default.xex` (local only; do not commit).
3. Run:

```powershell
powershell -File tools/tier0/run_smoke_roster.ps1
```

Writes `telemetry/smoke_stub_summary.json` when stub JSONL files exist.

## Capture (timed smoke)

```powershell
.\tools\tier0\run_smoke_capture.ps1 `
  -TitleId 4D5307D1 `
  -GamePath "path\to\game.xex" `
  -XeniaExe build\bin\Windows\Release\xenia_canary.exe `
  -DurationSec 300 `
  -Hid xinput `
  -LogFile telemetry/4d5307d1_crash.log
```

## Post-stub crash triage (BF2 and others)

**Symptom-led order:** gradual gfx/physics decay, audio cut 1-2s before freeze -> check **PCM tail**, then **GPU log + frame trace**.

```powershell
# Play to crash (no auto-kill). -FreshSession clears stale logs. Debug exe used when built.
.\tools\tier0\launch_bf2_triage.ps1 -GamePath "D:\path\to\game.iso" -FreshSession

# After quit:
.\tools\tier0\triage_gameplay_capture.ps1 -TitleId 454107DB
```

**Do not** double-click `xenia_canary.exe` for triage — the config dump will show empty
`log_file` / `kernel_stub_hit_log` and F4 traces will not land under `telemetry/`.
**F4 frame traces require a Debug build** (`NDEBUG` disables the trace writer in Release).

Manual tools:

```powershell
python tools/tier0/analyze_pcm_tail.py telemetry/454107db_pcm.jsonl --tail-sec 120
python tools/tier0/analyze_pcm_boot.py telemetry/454107db_pcm.jsonl --boot-sec 120
python tools/tier0/triage_crash_log.py telemetry/454107db_crash.log
```

PCM full compare drifts during long gameplay; use **tail analysis** for crash signal, not full-file diff.

## Triage patches for smoke set

```powershell
python tools/tier0/list_smoke_patches.py --title-id 4D5307D1 5454082B --json-out telemetry/smoke_patches.json
```

See [61_tier1_gameplay_gate.md](../../Xenia-Phoenix/metacache/plan/61_tier1_gameplay_gate.md).

## Live smoke session (agent handoff)

1. Pre-flight: `powershell -File tools/tier0/smoke_session_ready.ps1`
2. Stub/XMA/PCM capture or `launch_bf2_triage.ps1` for crash runs.
3. `triage_gameplay_capture.ps1 -TitleId ...` writes `{title_id}_triage_report.txt`.
