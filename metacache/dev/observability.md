# Observability v2

Unified hub: `xe::obs` in `src/xenia/base/obs/`. Replaces juggling `log_level`, `log_mask`, stub-only JSONL, and ad-hoc probe strings as separate mental models.

## Presets (`log_preset`)

| Preset | Text log | JSONL events | Guest.Print | Gpu invariants | Probe |
|--------|----------|--------------|-------------|----------------|-------|
| `play` | warn | off | off | count only | off |
| `support` | info | on | off | count only | off |
| `homebrew` | info | on | info → `guest.log` | count | optional |
| `develop` | debug | on + stubs | debug | warn + event | port 8765 |
| `forensic` | debug | on | on | event | on + crash bundle |

CLI wins over TOML: `--log_preset=develop` for BF2 triage scripts (`launch_bf2_triage.ps1`, `gpu_repro_capture.ps1`).

Example config: `xenia-phoenix-src/xenia-canary.config.toml.example`.

## Telemetry artifacts

| Artifact | Path | When |
|----------|------|------|
| Crash log | `telemetry/{tid}_crash.log` | Always (instrumented launch) |
| Stub JSONL (legacy) | `telemetry/{tid}_stubs.jsonl` | develop / support |
| **Events JSONL** | `telemetry/{tid}_events.jsonl` | develop, support, homebrew, forensic |
| **Obs summary** | `telemetry/{tid}_obs_summary.json` | Post (`log summarize --write-summary`) |
| Guest log | `telemetry/{tid}_guest.log` | homebrew / forensic |
| Forensic bundle | `telemetry/{tid}_crash_bundle/` | forensic preset on shutdown |
| GPU traces | `telemetry/{tid}_gpu_trace/*.xtr` | F4 at repro |

## CONFIG DUMP

Boot crash log includes an `OBSERVABILITY` block: preset, `obs_session_id`, channel levels, `phoenix_debug_port`, stub/events/guest paths.

## Events

Schema: [schemas/obs_event_v1.json](../schemas/obs_event_v1.json)  
Canonical field list: [schemas/telemetry.md](../schemas/telemetry.md) §8.

Summarize:

```powershell
cd xenia-phoenix-src
phoenixctl log summarize --title-id 454107DB --write-summary
```

BF2 post (`bf2_gpu_session.ps1 -PostOnly` / `repro capture --post-only`) runs summarize automatically and prints `classification` when invariant codes dominate.

## Channels

`log_channel_overrides = "Gpu.Edram=debug,Kernel.Stub=info"` in TOML, or macro:

```cpp
XE_LOG_CHAN(obs::ChannelId::kGpuEdram, Debug, "...");
```

Develop preset can raise `Gpu.Edram` without `log_level=3` globally.

## Invariants

`obs::Invariant(code, channel, violated, detail)` — `violated=true` means failure mode fired.

| Preset | Behavior |
|--------|----------|
| play / support | Increment probe snapshot counter only |
| develop | JSONL event + optional `XELOGW` (budget per frame) |
| forensic | Always JSONL event |

BF2 seeds (see [U-GPU-001](../findings/universal/gpu_edram.md)):

- `DepthHostSidecarStale` — guest EDRAM touched while host depth sidecar stale
- `HostDepthTransferMismatch` — host depth dropped on depth transfer
- `GpuUploadRangeError` — invalid upload range (D3D12)

## Masks

`log_disable_mask` is an alias for `log_mask` (bit mask of `LogSrc` channels to **disable**).

## Homebrew

See `xenia-phoenix-src/docs/homebrew_debug.md` and `src/xenia/debug/xenia_guest_debug.h` (traps 20/26, r3/r4).

## Transition (dual-write)

| Legacy | Unified |
|--------|---------|
| `kernel_stub_hit_log` only | Still written; also `kind=Stub` in events JSONL |
| Probe ring kinds | Bridged to events JSONL via `obs::BridgeProbeEvent` |
| `XELOG*` only | Channels + invariants for structured triage |

## Tools

| Tool | Purpose |
|------|---------|
| `phoenixctl log summarize` | One-screen profile + BF2 classification |
| `tools/tier0/aggregate_obs_events.py` | CLI wrapper around summarize |
| `tools/tier0/obs_preset_smoke.ps1` | Smoke: develop preset creates events file |
