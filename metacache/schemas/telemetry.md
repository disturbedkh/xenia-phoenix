# Telemetry schema

All structured outputs from Phoenix tooling. Stable fields = comparable across runs.

## 1. Kernel stub-hit JSONL (`--kernel_stub_hit_log`)

One JSON object per line. Emitted by `LogKernelStubHit` (`src/xenia/kernel/util/stub_trace.cc`).

| Field | Type | Notes |
|-------|------|-------|
| `ts` | string | ISO-8601 timestamp at hit |
| `module` | string | "xboxkrnl.exe" / "xam.xex" / etc. |
| `export` | string | export name + optional sub-path qualifier |
| `title_id` | string (hex) | from current process state |
| `lr` | string (hex) | guest link register at the call site |
| `ctr` | string (hex) | guest count register |
| `r3` | string (hex) | guest GPR 3 (1st arg in PPC ABI) |
| `r4` | string (hex) | guest GPR 4 |
| `r5` | string (hex) | guest GPR 5 |
| `detail` | string | free-form annotation provided by the call site |

## 2. VMX128 fuzz output

`vmx128-fuzz.exe` prints a summary block on stdout:

```
opcode: vaddubm
seed: 42
iterations: 100000
divergences: 0
elapsed_ms: <ms>
```

On divergence:

```
opcode: vaddubm
divergence at iter <N>:
  inputs (hex VR a / VR b): ...
  expected (scalar):        ...
  observed (jit):           ...
exit code: 1
```

Recommended: redirect to file `vmx128_fuzz.log`.

## 3. GPU replay CI report

`tools/gpu_replay_ci/run.py` writes JSON to `--report-dir/report.json`:

```json
{
  "tool_version": "1",
  "ran_at": "iso-8601",
  "backends": ["d3d12", "vulkan"],
  "traces": [
    {
      "name": "title_X_intro_loop.xtr",
      "backend": "d3d12",
      "render_target_path": "rtv",
      "exit_code": 0,
      "elapsed_ms": 1234,
      "output_dir_sha256": "abc...",
      "frame_hashes": ["...", "..."]
    }
  ],
  "rtv_vs_rov_diffs": [
    { "trace": "title_X_intro_loop.xtr", "diff_count": 0 }
  ]
}
```

## 4. Patch-debt JSON

`tools/tier0/categorize_patches.py` writes:

```json
{
  "ran_at": "iso-8601",
  "totals": {"A": 0, "B": 0, "C": 0, "D": 0},
  "patches": [
    {
      "title_id": "0x...",
      "title": "...",
      "patch_name": "fix_jit_crash",
      "category": "C",
      "category_reason": "matched keyword 'jit' in name; address in known-buggy range",
      "file": "game-patches/title_id.patch.toml"
    }
  ]
}
```

## 5. APU XMA divergence JSONL (`--apu_xma_divergence_log`)

Emitted by `LogXmaDivergence` (`src/xenia/apu/util/apu_trace.cc`) from `xma_context_new.cc` and `xboxkrnl_audio_xma.cc`.

| Field | Type | Notes |
|-------|------|-------|
| `context_id` | number | XMA context id when known |
| `site` | string | e.g. `Decode`, `DecodePacket`, `XMASetInputBuffer0` |
| `detail` | string | free-form |
| `title_id` | string (hex) | optional, from kernel |
| `lr` | string (hex) | optional guest link register |

Aggregate: `python tools/tier0/aggregate_xma_divergences.py telemetry/<title>_xma.jsonl`

## 6. APU PCM hash JSONL (`--apu_pcm_hash_log`)

Rolling SHA-256 of stereo int16 down-mix per window (`OnSubmitFramePcm` in `xaudio2_audio_driver.cc`).

| Field | Type | Notes |
|-------|------|-------|
| `title_id` | string (hex) | guest title id when wired (0 if unknown) |
| `window_start_ms` | number | steady-clock window start |
| `sha256_pcm_mix` | string | lowercase hex |
| `sample_rate` | number | Hz |
| `channels` | number | 1 or 2 after down-mix |

Compare: `python tools/tier0/compare_pcm_hash_log.py <current> <baseline>`

Cvar `apu_pcm_hash_interval_ms` (default 1000) controls window size.

## 7. xma2-diff report

`xma2-diff.exe` writes JSON summary via `--xma2_diff_report_out` and optional `xma2_divergences.jsonl` lines with `fixture` + `detail`.

## 8. Observability events JSONL (`obs_events_log`)

Unified stream from `xe::obs::EmitEvent` (`src/xenia/base/obs/`). Schema: [obs_event_v1.json](obs_event_v1.json). Playbook: [dev/observability.md](../dev/observability.md).

| Field | Type | Notes |
|-------|------|-------|
| `v` | number | Always `1` |
| `ts_ms` | number | Host wall-clock ms |
| `session` | string | `obs_session_id` at boot (hex) |
| `title_id` | string (hex) or null | When title is known |
| `origin` | string | `host` (guest later) |
| `domain` | string | e.g. `Gpu`, `Kernel`, `Guest` |
| `channel` | string | e.g. `Gpu.Edram`, `Kernel.Stub` |
| `kind` | string | `Stub`, `Invariant`, `Event`, `Log` |
| `severity` | string | `error`, `warn`, `info`, `debug` |
| `code` | string | Stable code (`StubHit`, `DepthHostSidecarStale`, probe kind, …) |
| `detail` | string | Free-form; hot codes aggregated (first + every 64th) |
| `frame` | number | Optional GPU frame |
| `guest_lr` | string (hex) or null | Guest link register when known |

**Default path (develop preset):** `telemetry/{title}_events.jsonl` (e.g. `454107db_events.jsonl`).

**Sources (dual-write during transition):** `stub_trace.cc`, `phoenix_probe` bridge, GPU invariants, `TrapDebugPrint` → `Guest.Print`.

**Aggregate:** `phoenixctl log summarize --title-id <ID> [--write-summary]`

## 9. Post-session obs summary (`{title}_obs_summary.json`)

Written by `phoenixctl log summarize --write-summary` (also from `gpu_repro_capture.ps1 -PostOnly`).

| Field | Type | Notes |
|-------|------|-------|
| `title_id` | string | Uppercase hex |
| `events_path` | string | Relative path to JSONL |
| `event_count` | number | Lines parsed |
| `time_range_ms` | object | `min` / `max` from events |
| `top_codes` | array | `{ code, count }` |
| `domains` | object | Domain → count |
| `xtr_files` | array | Paths under `telemetry/{tid}_gpu_trace/` |
| `gates` | object | `launch_guard`, `visual_verify` (manual) |
| `classification` | object | **BF2 only** — `symptom_bucket`, `finding_ids` (`U-GPU-001`, `G-454107DB-003`), `dominant_codes`, optional `probe_gpu` |

## Conventions

- All timestamps are ISO-8601 UTC.
- All hex fields use `0x` prefix and lowercase digits.
- All file paths in JSON are relative to the canary fork root.
- All hashes are lowercase SHA-256 hex.
- New fields are additive; never repurpose an existing field.
