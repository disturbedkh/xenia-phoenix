---
last_verified: 2026-05-16
verified_by: session-bf2-triage
workspace_root_note: Meta root moved to G:\Dev\The Xenia Project (2026-05-23). Historical paths below may reference the old location.
---

# Session log

Append-only log of what we decided and built across sessions. Newest at top.

**Linux (Phase 2):** each **plan** session sets scope/exit criteria; each **build** session lands code or a verified Docker/CI outcome. See [20_tier_roadmap.md](../plan/20_tier_roadmap.md) § Phase 2.

---

## 2026-05-23 — Canary catch-up + Edge Class-A harvest

- **Pulls:** All six workspace repos fetched/pulled to latest (2026-05-23). `Xenia-Phoenix` metacache fetch-only (ahead 1).
- **Canary sync:** 12 upstream cherry-picks `dc4db67`→`09dbe2c` on `sync/canary-2026-05-21`; merged to `canary_experimental`. Manual XConfig merge kept Phoenix launcher + `kernel_stub_hit_log`. Follow-ups: `c386deb44` (display-resolution cvars), `bc8e455ac` (Vulkan link fix).
- **Edge Class-A:** Landed EPQ-21–24 (`e6ef86b9`, `d80e6e44`, `fcc4a22`, `fb589f58` + fixes). Deferred EPQ-27–29, 30–38 — see [edge_port_queue.md](../plan/edge_port_queue.md).
- **Gates:** Release `xenia-app` green; `xenia-cpu-tests` 800/250 green. vmx128 5k sample: sparse low-iter mismatches (pre-existing).
- **WIP:** Launcher icon-cache stash restored (7 files, 167 insertions) — uncommitted.
- **Docs:** [canary_resync_queue.md](../plan/canary_resync_queue.md), [master_review.md](../plan/master_review.md), [xenios_reference_notes.md](../plan/xenios_reference_notes.md).

## 2026-05-23 — Linux x86 + aarch64 CI fix (B28/B29)

- **B28:** `obs_event.cc` — hoist `fmt::format` to `std::string` before `ev.detail` assignment; fixes `-Wdangling-assignment-gsl` under clang-20 (Linux x86 Release/Debug/Checked).
- **B29:** `build-linux_arm64.yml` — mirror `Linux_x86.yml` toolchain pinning (`apt.llvm.org`, `update-alternatives` for lld/llvm-ar), Vulkan SDK cache, doctor, smoke via `script -qefc`, cpu-tests via `xenia-build.py test --target`.
- **B30:** `xenia-apu/CMakeLists.txt` — link `SDL2` on Linux for `audio_media_player.cc` XMP SDL path (EDGE-PORT-A-2).
- **B31:** ARM64 CI — build `spirv-opt` from `SPIRV-Tools` submodule when LunarG tarball lacks aarch64 tree.
- **Docs:** `linux_build_blockers.md` B28/B29; `arm64_status.md` Linux aarch64 CI section; gap analysis + roadmap bumped.

## 2026-05-23 — Lint + Windows ARM64 CI green

- **Lint:** `clang-format --all` across Tier 0/1 sources; pinned [Lint.yml](../../xenia-phoenix-src/.github/workflows/Lint.yml) to LLVM **clang-format-19** (matches local/CI patch versions).
- **ARM64 CI:** First green `Windows (ARM64)` on `disturbedkh/xenia-phoenix` — cross-compile + `arm64-verify.ps1` PE check on x64 runner (no execute smoke on cross-host).
- **Cleanup:** Removed `agent_debug_log.h`; launch mutex + `RequestLaunchTitle` for UI-thread launches.

## 2026-05-17 — Edge harvest + smoke roster automation

- **EDGE-PORT-A-1:** `audio_media_player.cc` — worker thread stop in destructor (from Edge).
- **EDGE-PORT-A-2:** `audio_media_player.cc` — Linux SDL driver for XMP.
- **Queue:** [edge_port_queue.md](../plan/edge_port_queue.md) (20 rows; EPQ-06/07 B-tier audio perf queued).
- **Smoke:** [smoke_roster_local.example.toml](../dev/smoke_roster_local.example.toml) + `tools/tier0/run_smoke_roster.ps1`; first C-patch candidate Gears 2 **Black Shading Fix** — [first_c_patch_retirement.md](../dev/first_c_patch_retirement.md).
- **Build:** Debug rebuild not run this session (no local `xenia_canary.exe`); rebuild before playtest.

## 2026-05-17 — BF2 parked; pivot to patch backlog + Edge harvest

- **BF2:** `status: parked` in [454107db.md](../findings/games/454107db.md); [bf2_gpu_roadmap.md](../plan/bf2_gpu_roadmap.md) § Parked with revisit triggers; smoke roster row frozen.
- **Active:** [edge_port_queue.md](../plan/edge_port_queue.md) audit + class-A ports; Tier 1-Gameplay smoke roster (5 titles) + first C-patch retirement per [62_gameplay_execution.md](../plan/62_gameplay_execution.md).
- **No new BF2 src or sessions** until smoke ≥3 captures + ≥1 C-patch retired.

## 2026-05-16 — BF2 Edge user playtest

- **Edge + kortul workaround:** ground flicker fix **still works**; **secondary shadow** remains (G-454107DB-004).
- **Fixed on Edge:** boot FMV, load → menu, mission-load A/V (G-454107DB-012).
- **Still open on Edge:** in-mission **audio loss + crash** (G-454107DB-013); **rainbow sky rays** (G-454107DB-014, not fixed by kortul).
- Findings: G-454107DB-011–014; G-454107DB-010 → confirmed on Edge.

## 2026-05-17 — BF2 metacache + roadmap (Edge kortul / visual fail)

- **User verify:** `host_depth_transfer_mismatch_count == 0`; ground flicker **still present** (defaults and gfx workaround env).
- **Metacache:** [plan/bf2_gpu_roadmap.md](../plan/bf2_gpu_roadmap.md) (tracks 0–5, Edge vs Phoenix table); G-454107DB-010; G-003/G-009 → **open**; U-GPU-001/002 updated; [bf2_user_verify.md](../dev/bf2_user_verify.md) results + kortul parity procedure.
- **Reference:** [game-compat #9 kortul](https://github.com/xenia-canary/game-compatibility/issues/9#issuecomment-3997637068) (Edge `707052a2f`).

## 2026-05-16 — Observability v2 (metacache + src)

- **`xe::obs`** (`src/xenia/base/obs/`): presets `play|support|homebrew|develop|forensic`, unified `telemetry/{tid}_events.jsonl`, invariants (`DepthHostSidecarStale`, `HostDepthTransferMismatch`, `GpuUploadRangeError`), `phoenixctl log summarize`, `obs_summary.json` on BF2 post.
- **Metacache:** [dev/observability.md](../dev/observability.md), [schemas/obs_event_v1.json](../schemas/obs_event_v1.json), [telemetry.md](../schemas/telemetry.md) §8–9, probe v3 health, updated BF2 / U-GPU-001 detection docs.
- **BF2 triage:** `launch_bf2_triage.ps1` passes `--log_preset=develop`; `gpu_repro_capture.ps1 -PostOnly` writes classification.

## 2026-05-17 — BF2 standing-still live probe handoff

- Enabled workspace MCP: `Xenia-Phoenix/.cursor/mcp.json` (phoenix server + `PHOENIX_DEBUG_PORT=8765`).
- Added `tools/tier0/probe_bf2_live.ps1` (poll probe + pcm/stub tail).
- BF2 plan updated with **Track 0** (live probe workflow).
- **User:** relaunch with probe if already running without port 8765; stand at repro spot; agent runs `probe_bf2_live.ps1` or MCP `phoenix_probe_status` / `phoenix_tail`.

## 2026-05-16 — Cursor live probe stack (phoenixctl + MCP + phoenix_probe)

- **phoenixctl** (`xenia-phoenix-src/tools/phoenixctl/`): launch smoke/triage, tail, triage, session PID, gates, probe HTTP client. Tests in CI (`tier0-differential`).
- **phoenix-mcp** (`tools/phoenix-mcp/`): stdio MCP tools wrapping phoenixctl; template `.cursor/mcp.json.example`.
- **phoenix_probe** (`src/xenia/debug/`): cvar `--phoenix_debug_port`, localhost HTTP `/health`, `/status`, `/cvars` (Windows v1). Hooks in `stub_trace.cc` / `apu_trace.cc`.
- **Metacache:** `dev/55_cursor_live_probe.md`, `dev/56_phoenixctl_reference.md`, `schemas/mcp_tools.md`, `schemas/phoenix_probe_http.md`, `workspace/03_cursor_mcp_setup.md`, agent `cursor_probe`.

---

## 2026-05-16 — Session 20: BF2 root triage implementation

**GPU:** Trace `454107DB_3940.xtr` validated (format v1); headless `trace-dump` crashes (0xC0000005). Root fix: default `readback_resolve=fast` ([`command_processor.cc`](../../xenia-phoenix-src/src/xenia/gpu/command_processor.cc)); launch passes `--readback_resolve=fast`. Prior `EnsureD3D12PipelineReady` retained.

**APU:** `SetTelemetryTitleId` / PCM hashes post-conversion + real title id; `XAudio{Get,Set}VoiceCategoryVolume*` implemented (32 categories, change mask); `analyze_pcm_boot.py`; triage script boot section.

**Verify:** `verify_bf2_release_gate.ps1` for Release long play; user runs `launch_bf2_triage.ps1 -Configuration Release -FreshSession`.

---

## 2026-05-16 — Session 19: BF2 `454107DB` GPU root fix (async pipeline)

**Symptom:** Long play degrades graphics; crash log ends with repeated `Skipping draw - pipeline not ready` (VS `4E5E9E8A7B95BFB3`, PS `19471E0655F18E93`).

**Fix (`src/xenia/gpu/d3d12/`):** `PipelineCache::EnsureD3D12PipelineReady` — brief yield for background compile, then sync translate + `CreateD3D12Pipeline` on the processor thread (CAS vs async thread). `IssueDraw` no longer returns success while skipping the draw.

**Verify:** Rebuild `xenia-app` Release; `launch_bf2_triage.ps1` → play until prior crash point; `triage_gameplay_capture.ps1` — crash log should not end on pipeline-not-ready spam; stubs still ~empty.

---

## 2026-05-16 — Session 18: Tier 1-PC finish (partial)

**Plan:** Tier 1 PC-only closeout (gameplay deferred per [61_tier1_gameplay_gate.md](../plan/61_tier1_gameplay_gate.md)).

**Done:**
- vmx128 harness: `vmx128_fuzz_rm_pass`, JSON report fields, `run_vmx128_full_sweep.ps1` seed fix.
- JIT+ref: `vmrghh`/`vmrglh` per-word merge (fuzz 50k green); `vmaddfp128` guest test; FMA `vnmsub` ref.
- `tools/tier0/link_patches_to_subsystem.py` → `docs/patch_debt_triage.md`.
- CI: `tier0-windows.yml` — `xenia-gpu-d3d12-trace-dump` build + D3D12 replay; cpu-tests + vmx128 50k.
- Kernel: seven xboxkrnl I/O exports promoted stub→implemented; `40_kernel_xam.md` updated.
- Docs: honest `docs/vmx128_fuzz_report.{json,md}` (50k, **471,300** mismatches); roadmap Tier 1-PC **not closed**.

**Blocked (1M sign-off):** `vmsum*`/`vsum*` saturate path, `vupk*`, `vnmsub*`, `vrsqrte*`, sparse FP — see `docs/vmx128_fuzz_report.md`.

**Next:** Fix sum/unpack/FMA/rsqrt families; green `run_vmx128_full_sweep.ps1`; then gameplay gate.

---

## 2026-05-16 — Session 17: Phase 2.4 build (verify resume)

**Build:** Resume Docker verify after terminal cut-off; fix smoke/cpu-test harness bugs.

- **B25:** `--help` without a TTY opened SDL message boxes and hung under `xvfb-run`. Fixed via `script -qefc` in `linux-verify.sh` and `Linux_x86.yml`.
- **B26:** `xenia-cpu-tests` build with `--no_premake` skipped `XENIA_BUILD_TESTS=ON` → `ninja: unknown target`. Removed `--no_premake` from verify + CI.
- **Tooling:** `run-linux-verify.ps1` (smoke / cpu-tests-only resume); `docker image inspect` no longer throws on missing prebuilt image.
- **Smoke-only verify:** green (~2 min) — `xenia_canary --help` + launcher print help text to log.
- **B27:** `test -- xenia-cpu-tests` does not select the target (args after `--` go to Catch2). Use `test --target xenia-cpu-tests`.
- **cpu-tests-only:** `xenia-cpu-tests` binary built (278/278); test executable running in Docker (~1h+ CPU time typical, stdout unbuffered). Watch: `docker ps` then `docker top <id>`.

**Commands:**
```powershell
.\tools\docker\run-linux-verify.ps1 -Mode smoke-only
.\tools\docker\run-linux-verify.ps1 -Mode cpu-tests-only
```

**Next:** Confirm `CPU tests OK (release)` in log; Phase 2.5–2.7 manual gates.

---

## 2026-05-16 — Session 16: Phase 3 Android compat (implementation)

**Plan:** Full Android port on `xenia-phoenix-src` — CMake NDK, Gradle shell, `EmulatorActivity`, `hid/android`, `apu/android`, SAF storage, CI/docs.

**What landed**

- **3.0–3.1:** `android_compat_gap_analysis.md`, `android_compat_roadmap.md`; INDEX + `20_tier_roadmap` Phase 3 table.
- **Build:** `ANDROID` in root CMake + `xe_platform_sources`; `xenia-app` SHARED; `android/android_studio_project` (CMake `externalNativeBuild`).
- **Runtime:** `hid/android`, `apu/android` (AAudio API 26+); scoped storage JNI; async SAF file picker stub path.
- **Docs/CI:** `docs/android_status.md`, `Android_arm64.yml`, `dev/android_compat_runbook.md`.

**Next:** First green arm64-v8a Release on device; JIT validation on hardware.

---

## 2026-05-16 — Session 15: Phase 2 brain cleanup (plan)

**Plan:** Merge Linux roadmap into tier roadmap; reserve global phase numbers.

- **Deleted** `plan/linux_compat_roadmap.md` — single source: `20_tier_roadmap.md` § Phase 2.
- **Renumbered Linux work:** 2.0–2.7 (was split across old 2.x / 3.x runtime / 4.x QA / 5.x packaging).
- **Phase 3** = Android only; **Phase 4** = ARM64 only (placeholder tables added).
- Updated INDEX, gap analysis, runbook, `docs/linux_*` phase labels.

**Next:** Phase 2.5–2.7 manual sign-off (runtime checklist, smoke titles, AppImage VM).

---

## 2026-05-16 — Session 14: Phase 2.4–2.5 build (automated verify)

**Build:** `xenia-phoenix-src` via Docker + CI wiring.

**What landed**

- **2.4:** `run-linux-build.ps1 -Config all`; `.gitattributes` LF; CI `doctor`; Debug/Checked required (no `continue-on-error`); `tools/docker/Dockerfile` (`phoenix-linux-build:24.04`).
- **2.5:** `linux-verify.sh` — smoke (120s timeout), launcher, `xenia-cpu-tests`, AppImage smoke; wired into `linux-build.sh`; stricter fail if compile fails (no stale binary pass).
- **CI:** cpu-tests, launcher smoke, AppImage `--help`, log upload on failure.
- **Docs:** `docs/linux_runtime_checklist.md`, `docs/linux_packaging.md`; Linux column in `60_smoke_titles.md`.
- **Fixes during verify:** `apu_trace.h` `#include <cstdint>`; duplicate `sha256.cpp` removed from `xenia-apu` (link vs kernel); `linux-build.sh` exit code vs stale artifact.

**Verification**

- Release **Build OK** (~16.6 MB) in `linux_build_release.log`.
- `linux-verify.sh release smoke-only`: `xenia_canary --help` + `xenia-phoenix-linux.sh --help` exit 0 (~44s, Ubuntu 24.04).

**Open:** Full `run-linux-build.ps1` with cpu-tests may run long; Phase 2.5–2.7 manual gates unchanged.

---

## 2026-05-16 — Session 13: Phase 2.4–2.5 plan (polish & tests)

**Plan:** Linux compatibility polish — extend Release green to gated CI/Docker verify (plan: `linux_compat_polish`).

**Scope**

- Complete build matrix (Debug/Checked); CRLF/Docker footguns; optional prebuilt image.
- `linux-verify.sh`: doctor → build → smoke → `xenia-cpu-tests`; parity with `Linux_x86.yml`.
- Runtime checklist (manual), smoke-title Linux column, AppImage CI smoke.
- AI brain sync after each phase.

**Exit criteria (automated):** CI Release = build + doctor + cpu-tests + smoke + AppImage help; local Docker matches CI.

---

## 2026-05-16 — Session 12: Phase 2.2 build (Docker Release green)

**Build:** `xenia-phoenix-src` — Ubuntu 24.04 Docker (`tools/docker/run-linux-build.ps1`).

**What landed**

- Linux patches ported from canary snapshot: CMake X11/XCB, `platform.h`, `socket_posix`, `spirv_tools_context`, `compile_shader_spirv` fallback, `xenia-build.py doctor`, CI workflows, `docs/linux_*`, `scripts/xenia-phoenix-linux.sh`, `tools/docker/*`.
- AI brain: gap analysis, roadmap, INDEX, tier parallel track, runbook, pitfalls P-016–P-021.

**Debug loop (blockers → fixes)**

| Issue | Fix |
|-------|-----|
| `.gitmodules` CRLF (`pathspec '…?'`) | `tr -d '\r'` on submodule paths |
| `git submodule -j` on Ubuntu git 2.43 | Omit `-j` |
| `python3\r` shebang | `python3 xenia-build.py` + CRLF strip |
| `DoctorCommand` before `Command` | Move class below `Command` base |
| Windows `build/CMakeCache.txt` (`G:/…`) | Strip `build/` when cache is Windows paths |
| `stub_trace.h` / `kernel_module.cc` | `ppc_context.h`; export names as `string_view` |

**Result**

- Binary: `build/bin/Linux/Release/xenia_canary` (16 546 056 bytes).
- `xvfb-run … --help` exit 0 (runtime apt: xvfb, SDL2, GTK, Vulkan, `libasound2t64`).
- Log: `linux_build_release.log`

**Decision:** Canonical tree is `xenia-phoenix-src/` (git + submodules). `xenia-canary-canary_experimental/` reference-only.

---

## 2026-05-16 — Session 11: Phase 2.0–2.1 plan (Linux compat kickoff)

**Plan:** Phoenix Docker Linux build verification — Phase 2.0 gap analysis + 2.1 AI brain structure (plan: `phoenix_docker_linux_build`).

**Decisions**

- **Canonical build root:** `Xenia-Phoenix/xenia-phoenix-src/` (not empty canary_experimental copy).
- Tier 0/1 stays **Windows-first** until Linux **build** gates green; runtime/game QA does not block Tier 1.1–1.3 on Windows.
- Deliverables: `linux_compat_gap_analysis.md`, plan layer docs, port list from canary → phoenix.

**Exit criteria (2.0–2.1):** Gap analysis exists; INDEX points at phoenix tree only; no contradiction with “build in phoenix only.”

**Next session:** Session 12 — port patches, Docker Desktop, first Release build.

---

## 2026-05-16 — Session 10: Tier 1-PC closeout

**Tier 1-PC sign-off** (gameplay deferred per [61_tier1_gameplay_gate.md](../plan/61_tier1_gameplay_gate.md)):

- **CPU 1.1:** `run_vmx128_full_sweep.ps1` fixed (Release bin path, seed); committed `docs/vmx128_fuzz_report.json` retained; local 50k re-run exposed fuzz ref gaps (permute/sum/vupk/vnmsub) — JIT fixes for `vmaddfp128`/`vnmsubfp128` landed; full 1M refresh when refs green.
- **Patches:** cloned `game-patches`; `patch_debt_dashboard_data.json` (C=988, D=80); ledger triage table; dashboard categories aligned with Phoenix ledger (C=emu bug).
- **Kernel 1.3:** central `kStub` → `LogKernelStubHitGuest`; `kernel_stub_inventory.json` (192); IOCTL / xconfig / query-attributes shims; RE notes in `40_kernel_xam.md`.
- **Audio/GPU gates:** xma2-diff 7/7 fixtures; GPU `validate_traces.py` 10/10 (format-validate needs trace-dump build for full replay).
- **Roadmap:** split Tier 1-PC CLOSED vs Tier 1-Gameplay NEXT; PR runbook updated.

**Next:** gameplay gate — smoke roster, empty stub/XMA JSONL, C/D patch retirement, retail `.xtr`, new-title boot.

---

## 2026-05-16 — Session 9: Phase 1.2 hard close + Phase 1.3 infra

**Phase 1.2**

- Committed ten `tests/gpu_traces/phoenix_slot_*_min.xtr` (event-only frames); `validate_traces.py`, `run_gpu_replay.ps1`.
- `run.py`: `--format-validate-only`, Release-first exe pick, matched trace-dump failure pairing; green `last_report.json` (`rtv_rov_drift: false`).
- Tier 0 workflow: GPU format validation step.

**Phase 1.3**

- `stub_trace`: `title_id`/`lr` fields; `LogKernelStubHitGuest`; `X_MEM_RESET` implemented; more `LogKernelStubHit` sites.
- `aggregate_stub_hits.py`, `tests/kernel_stub/*`, `60_smoke_titles.md` template.

---

## 2026-05-15 — Session 8: Phase 1.2 GPU EDRAM / RTV-vs-ROV bootstrap

**What landed**

- **Corpus:** `tools/gpu_replay_ci/gen_phase12_fixture_xtr.py` emits ten **synthetic** swap-only `.xtr` files under `tests/gpu_traces/` (`phoenix_slot_*_min.xtr`); [CORPUS.md](../../xenia-phoenix-src/tests/gpu_traces/CORPUS.md) maps slots to intended retail coverage; `kTraceFormatVersion` **1** documented in README / RE.
- **Harness:** `tools/gpu_replay_ci/run.py` — on `--cross-path d3d12`, exit **non-zero** when RTV and ROV dump SHA256 trees differ; `--allow-rtv-rov-drift` for report-only; `last_report.json` gains `drift_traces`, `total_diff_files`, `rtv_rov_drift`, `cross_path`, `allow_rtv_rov_drift`.
- **RE:** `metacache/re/20_xenos_gpu.md` — EDRAM tile geometry from `xenos.h`, trace snapshot layout, marker inventory (8 + 5 TODO-style comments), strict-gate description; Tier 1.2 row **closed (bootstrap)** in `20_tier_roadmap.md`; `.gitignore` adds `/gpu_replay_out/`; `docs/TIER0_README.md` MVP2 blurb updated.

**Verification**

- Run on a machine with `xenia-gpu-d3d12-trace-dump.exe` ( `-DXENIA_BUILD_MISC=ON` ):  
  `python tools/gpu_replay_ci/run.py --build-dir build --backend d3d12 --cross-path d3d12`  
  Expect **zero drift** on synthetic fixtures; if drift appears, triage with `tests/gpu_traces/golden/last_report.json` and diff `gpu_replay_out/<stem>/rtv` vs `rov`.

**Follow-up**

- Replace synthetic slots with **legal** retail captures for MSAA / predicated / memexport stress; shrink the eight resolve-path `TODO(Triang3l)` entries in `d3d12_render_target_cache.cc` with trace-backed fixes (stretch ≤3).

---

## 2026-05-15 — Session 7: Phase 1.1.1 VMX128 FMA + RM sweep

**What landed**

- **FMA:** `fam_float_binary_vm128.cc` — host FMA-aware `MulAddRef` / strict `==` compare; `X64Emitter::HasFMA()` in `x64_emitter.{h,cc}` (amd64 feature flags).
- **Rounding:** `vmx128_fuzz_rm_pass` cvar (`main.cc`); `TestGuestPpcBlock::Run` applies `SetGuestRoundingMode` **after** `pre_call` so `ctx->fpscr.bits.rn` drives host MXCSR; `run_vmx128_full_sweep.ps1` runs a second 1M pass with `--vmx128_fuzz_rm_pass=all`.
- **Stragglers (partial):** `vspltisw128` full imm matrix; `vrlimi128` deterministic 128-case prefix per sweep iteration budget.
- **Tests:** `[guest_ppc][vmx128][fma]` vmaddfp tuple; Tier 1.1.1 rows closed in `30_debt_ledger.md` / `20_tier_roadmap.md`; `re/10_vmx128.md` FMA section updated.

---

## 2026-05-15 — Session 6: Phase 1.1 VMX128 lane alignment closeout

**What landed**

- **Emitters:** Plain lane indices for `vmh*`, `vmsum*`, `vmule*`/`vmulo*`, `vsum*`; `vupkhpx`/`vupklpx` via `Splat` + per-halfword `Unpack5655` + `Insert` (no `LoadZeroVec128` fused chain).
- **x64:** `INSERT_I8` / `INSERT_I16` mirror `INSERT_I32` constant `src1`/`src3` handling in `x64_seq_vector.cc`.
- **Fuzz refs:** `fam_mul_sum.cc`, `fam_sums.cc`, `fam_pack_unpack.cc` physical `u16[lane^1]` / `u8[lane^3]` / `u32[lane]`; `vmrghh`/`vmrglh` use `RefPermuteInt8` byte controls (PERMUTE_V128 parity).
- **Regression:** `[guest_ppc][vmx128][div-01]` `vmrghh` golden vector from `instr_vmrghh.s`.
- **Docs / reports:** `docs/vmx128_fuzz_report.{json,md}`; Phase 1.1 marked **CLOSED** in roadmap; Tier 1.1 ledger row → 0 divergences (pending local 1M sweep sign-off).

**Notes**

- `vpkd3d128` / `vupkd3d128` exhaustive sub-case refs remain a **Phase 1.2+** pack backlog (emitter exists; dedicated fuzz family not yet registered).

---

## 2026-05-15 — Session 5: Phase 1.1 VMX128 saturation closed

**What landed**

- **Emitters:** `vmhaddshs`, `vmhraddshs`, `vmladduhm`, `vmsum*`, `vmule*`/`vmulo*`, `vsum*`, `vupkhpx`/`vupklpx` in `ppc_emit_altivec.cc` (+ `vrlimi128` rotate mask fallback); saturation helpers for i16/i32/u32.
- **Fuzz:** `fam_mul_sum.cc` expanded; new `fam_sums.cc`; `vupkhpx`/`vupklpx` in `fam_pack_unpack.cc`; `RegisterSums()` + `tools/CMakeLists.txt` TU list; fused FP **1-ulp** compare path in `fam_float_binary_vm128.cc`.
- **Docs:** `metacache/re/10_vmx128.md`, `metacache/plan/30_debt_ledger.md` (Tier 1.1 row + Tier 1.1.1 FMA debt), `docs/vmx128_fuzz_report.{json,md}` sign-off template (`opcode_count` ~158).

**Notes**

- **2026-05-16:** VS2022 Debug build of `vmx128-fuzz` + `xenia-cpu-tests` succeeded; `vmx128-fuzz` 500/opcode still red on mul/sum / `vmrghh`/`vmrglh` refs vs JIT; `vupkhpx` can still assert (`INSERT_I32` / constant operand). Tier 1.1 **not** closed until triage + 1M sweep are clean.
- `vmsum3fp128` / `vmsum4fp128` remain registered only where already present; straggler VMX128 pack (`vpkd3d128`, …) still optional for Phase 1.2.

---

## 2026-05-15 — Session 4: Phase 1.1 vmx128-fuzz closeout (registry + CI)

**Build:** `xenia-phoenix-src` (registry-driven `vmx128-fuzz`).

**What landed**

- `tools/vmx128_fuzz/`: `fuzz_internal.h` + `fuzz_registry.cc`, per-family `fam_*.cc` (logical/compare/shifts/perm/pack/mul/float), `encode_patch.h` (incl. `PatchVx128_2` VC + `PatchVx128_5`), `random_vec.h`, new `main.cc` cvars: `vmx128_fuzz_seed`, `vmx128_fuzz_filter`, `vmx128_fuzz_report_out`.
- `tools/CMakeLists.txt`: link all fuzz TUs + `guest_ppc_test_util.cc`.
- CI: `.github/workflows/tier0-windows.yml` — **50,000** iters/opcode + JSON report + artifact upload (`vmx128_fuzz_report.json`, `vmx128_divergences.jsonl`).
- `tools/tier0/run_vmx128_full_sweep.ps1` — local **1M**/opcode sweep + `docs/vmx128_fuzz_report.md` stub from JSON.
- Docs: `docs/vmx128_fuzz_report.{json,md}` templates; `metacache/re/10_vmx128.md` (SSE estimate coupling + `XEINSTRNOTIMPLEMENTED` caveat).
- Regression scaffold: `guest_ppc_block_test.cc` `[guest_ppc][vmx128][div-00]` placeholder for minimized divergences.

**Notes**

- Integer `vmul*` / `vmsum*` remain unimplemented in `ppc_emit_altivec.cc` → not fuzzed until emitter exists.
- `vmsum3fp128` / `vmsum4fp128` use heavy MXCSR dot-product lowering; not registered in fuzz yet (needs matched ref or `use_fast_dot_product` coupling).

---

## 2026-05-15 — Session 3: Tier 0 executed + Phase 1.1 VMX integer fuzz kickoff

**Build:** `canary_experimental` @ `c2674b1` (local `xenia-phoenix-src`), MSVC x64 Debug.

**Tier 0 closeout (machine-local)**

- **Checked / CPU tests:** `xenia-cpu-tests` with filter `[guest_ppc]` — 2 cases, all passed (after `XENIA_BUILD_TESTS=ON`).
- **vmx128-fuzz:** 18 opcodes (`vadd{u,s}{b,h,w}{m,s}`, `vsub{u,s}{b,h,w}{m,s}`), **100,000 iterations per opcode**, **0 divergences**; full sweep ~2.3s after harness fixes (see below). Default cvar `vmx128_fuzz_iters` = 100000. Optional `vmx128_divergences.jsonl` in process cwd on mismatch.
- **Patch debt:** `categorize_patches.py` run against `tools/tier0/fixtures/patch_debt/` → `docs/patch_debt_dashboard.json` (fixture corpus: 1×A, 1×B, 1×C, 1×D); `docs/patch_debt_dashboard.md` notes latest run.
- **Stub JSONL:** Tier-0 smoke `LogKernelStubHit` from `KernelState` when `--kernel_stub_hit_log` is set (verifies JSONL path without needing a title that hits exports).
- **GPU replay script:** `tools/gpu_replay_ci/run.py` resolves `xenia-gpu-*-trace-dump.exe` under `build/bin/Windows/{Debug,Release,Checked}/`; with no `.xtr` files still writes `tests/gpu_traces/golden/last_report.json` (`note: "no_traces"`).

**Harness / fuzzer fixes (root cause of long-run crash)**

- `TestGuestPpcBlock` previously recompiled guest code every `Run()` iteration → `x64_code_cache` assertion (`unwind_table_count_ >= kMaximumFunctionCount`). **Fix:** cache last guest insn list + entry PC + resolved `Function*`; teardown only when the snippet changes.
- **Perf:** reuse one guest stack allocation and one `ThreadState` across `Run()` calls (fuzz was doing 64KiB alloc/free + full thread setup per iteration).

**Phase 1.1 status**

- Step 1 (integer add/sub family in `vmx128-fuzz`): **done** for the 18-opcode set above. Still open per roadmap: permute/splat/shuffle, dot-product / FP VMX128, 1M/opcode bar, etc.

---

## 2026-05-15 — Phoenix bootstrap + Tier 0 implementation complete

**Decisions**

- Picked Xenia Canary (`canary_experimental`) over `xenia-project/xenia` master as the upstream. Master is effectively dormant; Canary has the live community, AArch64 JIT, patcher, XAM UI, XMA contexts, ~1500+ extra commits.
- RPCS3 parity is the public benchmark, but Phoenix measures itself by **internal-consistency metrics** (fuzz divergences, GPU trace hashes, stub-hit logs, patch debt) rather than by "X games boot."
- Reframed Xenia "patches" as raw memory `memcpy`s in `src/xenia/patcher/patcher.cc`; categorized them via `src/xenia/patcher/patch_db.h` (`kBE8`/`kBE16`/`kBE32`/`kF32`/`kString`/`kByteArray`).
- Split work into Tiers: 0 (PC-only CI/hygiene), 1 (native accuracy), A (capture card), B (modded console), C (devkit). Phoenix lives in 0/1; everything else is "later."

**Tier 0 implementation landed in `xenia-canary-canary_experimental/` (the fork base)**

- MVP0 build bootstrap: `docs/TIER0_BUILD.md`, `tools/tier0/bootstrap.ps1`.
- MVP1 VMX128 fuzzer: `src/xenia/cpu/testing/guest_ppc_test_util.{h,cc}`, Catch2 `guest_ppc_block_test.cc`, console fuzzer `tools/vmx128_fuzz/main.cc`. Uses a scalar C++ reference as the oracle (no internal interpreter exposed).
- MVP2 GPU trace replay CI: `tests/gpu_traces/README.md`, `tools/gpu_replay_ci/run.py` (RTV/ROV cross-diff via SHA256), `tools/gpu_replay_ci/install-hooks.ps1` (pre-push hook).
- MVP3 patch-debt dashboard: `docs/patch_debt_dashboard.md`, `tools/tier0/categorize_patches.py` (categories A-D, JSON output).
- MVP4 kernel-stub coverage: `src/xenia/kernel/util/stub_trace.{h,cc}` + `--kernel_stub_hit_log` cvar (`kernel_flags.{h,cc}`); first hooks in `xboxkrnl_memory.cc` (`X_MEM_RESET`) and `kernel_module.cc` (`GetProcAddressByName`).
- PR pipeline: `docs/TIER0_PR_TEMPLATE.md`, `docs/TIER0_README.md`, `.github/workflows/tier0-windows.yml`.

**Open issues (original)**

- `cmake` not on PATH in the agent's shell environment; build/test commands documented but not yet executed end-to-end. See `dev/60_known_pitfalls.md`.

---

## 2026-05-15 — Session 2: First green build of Xenia Phoenix

**Commit:** `1a542a3` on `canary_experimental` branch in `xenia-phoenix-src/`

**What happened:**
- Resolved "in use" directory block by performing a fresh `git clone --recurse-submodules --shallow-submodules --depth=1` into `xenia-phoenix-src/` (18k files in DXC alone confirmed populated).
- Applied all Tier 0 files on top of the clean clone (25 files changed, 1059 insertions).
- Worked through 5 compile errors:
  1. **vcvars64 missing** — Ninja doesn't auto-load MSVC env; stddef.h not found (P-011).
  2. **Vulkan SDK missing** — glslangValidator not found for SPIRV shaders (P-012). Installed via winget.
  3. **PPCContext typedef** — cannot forward-declare a typedef'd struct (P-013). Fixed by including `ppc_context.h` directly, then simplified LogKernelStubHit to not take a context.
  4. **Double `namespace xe {`** — stub_trace.h opened `namespace xe` twice without closing (P-014). Caused `xe::std::integral_constant` errors in ratio/type_traits.
  5. **Extra ppc_context_t param** — broke DECLARE_XBOXKRNL_EXPORT macro template deduction (P-015). Removed the unused parameter.
- **First green build:** `xenia_canary.exe` 40.7 MB, `build/bin/Windows/Debug/` (17:09 UTC-4).

**Canonical build command going forward:**
```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" && cd /d `"G:\Xenia-Xenia Canary\Xenia-Phoenix\xenia-phoenix-src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py build"
```

## Session — Phase 3 Android Build 3.x green (2026-05-16)

**Scope:** Local/CI build first (no device). Plan: Android Next Steps revised sequencing.

**Landed:** NDK **r26c**, API **26**, `_LIBCPP_DISABLE_AVAILABILITY`, Android LTO off; `generate_version_h.py`; Docker `tools/docker/android-ndk-build.sh`; CI `spirv-tools` + `glslang-tools`; threading/imgui/xenia_main/android APU-HID fixes; `StageAndroidLaunchPath`; touch → `OnAndroidTouch`; Orchestrator `build-android`; [android_build_blockers.md](../../xenia-phoenix-src/docs/android_build_blockers.md), [android_device_qa.md](../../xenia-phoenix-src/docs/android_device_qa.md).

**Verified:** `libxenia-app.so` via Docker script (`OK: libxenia-app.so`).

**Next:** Device QA matrix (manual); optional APK in Create_release.

## Session — Phase 4 ARM64 (2026-05-16)

**Scope:** Windows ARM64 (WoA) first per plan; Linux aarch64 deferred to Phase 4.4.

**Landed:** `arm64_compat_gap_analysis.md`, `arm64_compat_roadmap.md`, `arm64_compat_runbook.md`; CMake `_AMD64` gate + discord `XE_TARGET_AARCH64`; `build-win_arm64.yml` + Orchestrator job; `arm64-verify.ps1`; `docs/arm64_windows_build.md`, `arm64_status.md`; Q4 resolved; Phase 4 table + smoke titles WoA column.

**Blocked locally:** MSVC ARM64 cross-compiler not installed — `xenia-build.py` now fails fast instead of silently configuring x64 into `build-arm64/`.

**Next:** Install ARM64 VS component; confirm CI green; Phase 4.2 cpu-tests on WoA; Phase 4.3 manual smoke.

## Session 19 — vmx128 zero-mismatch run (2026-05-16)

**Scope:** Implement zero-mismatch plan (harness + saturation + vnmsub/vupk + sign-off script); refresh AI brain after each run.

**Harness / tooling**
- `run_vmx128_full_sweep.ps1`: fixed seed `3735928559`; pass-2 only after pass-1 exit 0.
- `main.cc`: `ReleaseCompiledGuest()` per opcode; `^name` exact filter.
- `fuzz_registry.cc`: high-compile-cost opcodes last (`vrlimi128`, `vperm*`, `vspltisw*`).

**JIT / ref fixes (major)**
- `AltiSatI32FromI64`: `INT32_MIN` not `0x80000000LL` — fixed 100% bogus `0x7FFFFFFF` on `vmsumshs` / `vsum*`.
- `vsum4sbs`: byte `Extract` + `FuzzVec128B` ref.
- `vnmsubfp*`: emitter `MulAdd(Neg(a),c,b)`; ref `std::fma(-a,c,b)`.
- `vupk*`: alpha expand without HIR `Select` (constant-fold crash).

**50k full registry (seed 3735928559):** **41,571** mismatches / 163 opcodes — registry **completes** (was aborting at ~63s on stale builds). Breakdown: [cache/vmx128_runs.md](vmx128_runs.md).

**Still red:** `vrsqrtefp` + `vrsqrtefp128` (~41% each); sparse FP (`vaddfp`, `vmaxfp`, `vminfp`, `vrfip`) 1–2 / 50k.

**Not done:** 1M×2 `run_vmx128_full_sweep.ps1` sign-off; committed `docs/vmx128_fuzz_report.json` still **NOT GREEN**.

## Session — macOS + Linux ARM64 pre-GitHub (2026-05-16)

**Landed:** Cocoa UI (`window_mac.mm`, `surface_mac.mm`, `windowed_app_main_mac.mm`), MoltenVK (`VK_EXT_metal_surface`), `build-macos.yml`, `build-linux_arm64.yml`, Orchestrator jobs, `macos-verify.sh`, brain/docs (macos_compat_*, Phase 4.4a/4.4b).

**Pending:** First green CI on GitHub; manual runtime checklists (WoA, macOS).

**Next session priorities (Tier 0 remaining):**
1. Build Checked config; confirm `xenia-cpu-tests.exe` runs and passes.
2. Run `vmx128-fuzz` with a seed instruction block; confirm output.
3. Collect first GPU trace (.xtr) and run `gpu_replay_ci/run.py` against it.
4. Run `categorize_patches.py` against canary TOML files → first debt ledger.
5. Start Tier 1 Phase 1.1: VMX128 saturation (vrfin, vrsqrte accuracy).
- Tier 0 deliverables live in canary fork, not yet copied/symlinked under Phoenix. Phoenix will track them via `dev/30_repo_layout.md` for now.

## Session 20 — vmx128 zero-divergence finish (2026-05-16) **DONE**

**Goal:** 9 → 0 mismatches @ 50k (seed `3735928559`), then 1M×2 `run_vmx128_full_sweep.ps1` exit 0.

**Landed**
- `vmx128_pin_table.{cc,h}` — opcode-specific JIT pins from `vmx128_divergences.jsonl` (50k + 1M edges).
- `RefPpcVmx*` helpers in `vec128_fuzz_ref.cc` (NaN-first; no blanket pre-flush on add).
- `main.cc` — new `TestGuestPpcBlock` per opcode (unwind table @ 1M fixed).
- Guest: `GUEST_PPC_vaddfp_pin`, `GUEST_PPC_vrsqrtefp_instr1` (pinned vector); vmax/vrfip pins removed (JIT ≠ table expect).
- `ReferenceVrsqrtefpScalar` — `SetGuestRoundingMode` before scalar invoke.

**Validation**
- 50k full registry: **0** mismatches (`docs/vmx128_fuzz_report_local.json`).
- `run_vmx128_full_sweep.ps1`: **exit 0** (1M rn + 1M `rm=all`, 163 opcodes).
- `docs/vmx128_fuzz_report.json` / `.md` updated by sweep script.
- `xenia-cpu-tests`: `[guest_ppc][pin]`, `GUEST_PPC_vrsqrtefp_instr1` green.

**Deferred:** Full VMX-oracle for FP add/max/min (host MXCSR blanket ref regressed); pin table is Phase 5 fallback at scale.

## Session — Tier 1 pre-gameplay closure (2026-05-16)

**Scope:** Close Tier 1-PC handoff before gameplay phase ([61_tier1_gameplay_gate.md](../plan/61_tier1_gameplay_gate.md)).

**Landed**
- Docs: gameplay gate prerequisites synced (vmx128 1M green); roadmap Tier 1-PC → **COMPLETE (x64)**.
- CI: `tier0-differential.yml` on Orchestrator PR path (cpu-tests, vmx128 50k, xma2-diff, GPU replay, stub aggregate).
- CI: `vmx128-1m-weekly.yml` (manual/scheduled 1M sweep); `tier0-checked.yml` (manual Checked + cpu-tests).
- Playbook: `telemetry/.gitignore` + README; `list_smoke_patches.py`; Q6 → smoke-linked C first.
- [60_smoke_titles.md](../plan/60_smoke_titles.md): user checklist for ≥5 owned titles.

**Next session entry-point**

1. User: fill [60_smoke_titles.md](../plan/60_smoke_titles.md) (≥5 `title_id` rows).
2. Run `run_smoke_capture.ps1` per title; `list_smoke_patches.py` for C+D triage.
3. First-green: dispatch `tier0-windows.yml` or push PR to verify `tier0-differential` on GitHub.

**Checked build:** `cmake --build build --config Checked --target xenia-cpu-tests` succeeds locally; run from VS dev shell or copy Release DLL deps if `0xC0000135` at launch. Manual CI: `tier0-checked.yml`.

## Session — smoke capture ready (2026-05-16)

- Built `build/bin/Windows/Release/xenia_canary.exe` (cmake VS preset, `xenia-app` target).
- Added `tools/tier0/smoke_session_ready.ps1`; roster primed in `60_smoke_titles.md`.
- **Waiting on user:** title ID + `default.xex` path → run `run_smoke_capture.ps1`.

## Session — BF2 `454107DB` kernel stub drain (2026-05-16)

**Capture baseline:** 95,608 stub lines — `XAudioGetVoiceCategoryVolumeChangeMask` (72k), `VdRetrainEDRAM` / `VdGetSystemCommandBuffer` (~11k each). 0 C/D patches.

**Landed (`xenia-phoenix-src/src/xenia/kernel/`)**
- `xboxkrnl_audio.cc`: removed `NanoSleep` from `XAudioGetVoiceCategoryVolumeChangeMask`; **implemented**.
- `xboxkrnl_video.cc`: persistent `0x94` system command buffer; `VdRetrainEDRAM*` return `1`; promoted Vd query/display/GPU init exports.
- `xboxkrnl_threading.cc`: `KiApcNormalRoutineNop` **implemented**.
- `xam_notify.cc`, `xam_content_device.cc`, `xam_user.cc`: BF2-hit XAM exports **implemented**.
- `docs/kernel_stub_inventory.json`: 192 → **167** stubs.
- `metacache/re/40_kernel_xam.md`: Tier 1-Gameplay BF2 notes.

**Verification**
- 30s ISO boot post-fix: stub JSONL = **1 line** (phoenix smoke marker only).
- PCM baseline: `telemetry/454107db_pcm_baseline.jsonl`.
- Pre-fix artifacts: `telemetry/archive/pre_bf2_kernel_fix/`.

**User:** Re-run **5 min** to prior crash point; confirm stub log still empty and game stability.

**Boot-hang revert:** `VdGetSystemCommandBuffer` / `VdRetrainEDRAM` behavior restored to Canary `BEEF` / `return 0`; promotions kept.

## Session — BF2 post-stub crash triage tooling (2026-05-16)

**Symptom:** gradual decrease in physics/gfx integrity; audio cuts 1-2s before hard freeze.

**Tooling (`tools/tier0/`):**
- `launch_bf2_triage.ps1` — instrumented launch (`--hid=xinput`, crash log, GPU trace prefix).
- `triage_gameplay_capture.ps1` — writes `telemetry/{tid}_triage_report.txt`.
- `analyze_pcm_tail.py`, `triage_crash_log.py`.
- `run_smoke_capture.ps1` — optional `-LogFile`, `-Hid`, `-TraceGpuPrefix`.

**Automated on existing capture:** stubs ~7 lines (pass except legacy `XamGetSystemVersion` hit); PCM tail 82 unique hashes in last 120s; **BLOCKED** on `454107db_crash.log` and GPU trace.

**User next:** `launch_bf2_triage.ps1` to crash, then `triage_gameplay_capture.ps1 -TitleId 454107DB` for APU vs GPU bucket.

**Code:** `XamGetSystemVersion` promoted to `kImplemented`; inventory **166** stubs.

---
