# GPU fix bar (game finding → universal src)

Minimum checklist before marking a GPU finding **resolved-native** in [findings/](../findings/INDEX.md).

## Discovery session (run first)

One command through the proof-loop door:

```powershell
# Run from xenia-phoenix-src (do not cd xenia-phoenix-src again if you are already there).
$env:PHOENIX_DEBUG_PORT = "8765"

# Launch + poll at repro, then post gates (--json works before or after subcommand):
phoenixctl repro capture --title-id 454107DB --game "D:\Xbox 360\Games\Battlefield 2 - Modern Combat (USA).iso" --launch --json

# After manual play + F4, gates only:
phoenixctl repro capture --title-id 454107DB --post-only --json

# In-game only (enable before entering level, not at main menu):
# $env:PHOENIX_BF2_GFX_WORKAROUND = "1"
```

Or: `powershell -File tools/tier0/gpu_repro_capture.ps1 -TitleId 454107DB -Launch`

Engineering harness (corpus symlink, build, replay, baseline):

```powershell
powershell -File tools/tier0/bf2_native_engineering.ps1 -All
```

See [bf2_user_verify.md](bf2_user_verify.md) for verify results (2026-05-17: mismatch 0, flicker **still present**). Roadmap: [bf2_gpu_roadmap.md](../plan/bf2_gpu_roadmap.md).

MCP: `phoenix_repro_capture`

## Probe counters (EDRAM translation)

At graphics repro, expect **non-idle** values vs menu (standing still on bad ground):

| Field | Rising means |
|-------|----------------|
| `ownership_change_count` | EDRAM tile handoffs with copies |
| `edram_transfer_count` | Individual transfer regions |
| `host_depth_store_count` | Host float32 depth sidecar writes (D3D12) |
| `host_depth_transfer_mismatch_count` | Host depth dropped (same RT as color) — [U-GPU-001](../findings/universal/gpu_edram.md) |
| `upload_range_error_count` | Stale page access on upload — [U-GPU-002](../findings/universal/gpu_edram.md) |

Workaround run (BF2 default triage cvars) vs `PHOENIX_BF2_GFX_WORKAROUND=1`: compare mismatch + upload counts and visual flicker/halo.

## Edge kortul parity (before closing U-GPU-001)

Community proof is on **Xenia Edge**, not Phoenix. Run explicit:

`depth_float24_convert_in_pixel_shader=true`, `gpu_allow_invalid_upload_range=true`, `depth_resync_on_guest_edram_touch=false`.

**Pass:** flicker gone (expect halo). **Fail on Phoenix only:** diff pipeline/float24 PS vs Edge — [bf2_gpu_roadmap.md](../plan/bf2_gpu_roadmap.md) Track 3.

Native fix bar **not met** until defaults visual pass **or** documented acceptance of workaround + U-GPU-003 halo plan.

## One-command BF2 workflow (recommended)

```powershell
cd xenia-phoenix-src
powershell -File tools/tier0/bf2_gpu_session.ps1 -Launch
# play → F4 at repro → quit
powershell -File tools/tier0/bf2_gpu_session.ps1 -PostOnly -Triage
```

`phoenixctl guard check --title-id 454107DB` must pass before trusting GPU triage.

**Observability v2:** `-PostOnly` writes `telemetry/454107db_obs_summary.json` and prints BF2 `classification` (graphics / U-GPU-001) when invariant codes dominate. One command: `phoenixctl log summarize --title-id 454107DB --write-summary`.

## Required (native fix)

1. **Repro without title launch cvars** (e.g. no BF2 `depth_float24` / `gpu_allow_invalid` unless testing workaround only).
2. **Legal in-game `.xtr`** at symptom (filename noted in game finding).
3. **`phoenixctl trace validate`** passes on that trace.
4. **`run_gpu_replay.ps1`** on trace (or new synthetic fixture) — RTV/ROV per [20_xenos_gpu.md](../re/20_xenos_gpu.md) policy.
5. **Drift log row** in [re/20_xenos_gpu.md](../re/20_xenos_gpu.md) with fix summary.
6. **Universal pattern** updated: `U-*` status → `fix-landed`; link PR/commit in finding.

## Discouraged

- Title-only `#ifdef` on `title_id` without detectable guest behavior
- Permanent cvar defaults in `launch_*_triage.ps1` as the only fix
- Closing compat issues without community repro steps

## Tools

```powershell
phoenixctl launch triage --title-id <ID> --game "<path>" --config Debug
phoenixctl repro capture --title-id <ID> --post-only --json
phoenixctl log scan --title-id <ID>
phoenixctl log summarize --title-id <ID> --write-summary
phoenixctl trace validate telemetry\<tid>_gpu_trace\<file>.xtr
powershell -File tools/tier0/run_gpu_replay.ps1
```
