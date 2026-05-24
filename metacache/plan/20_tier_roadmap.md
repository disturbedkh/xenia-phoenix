# Tier roadmap

The plan, ordered. Phoenix is currently at **Tier 1-Gameplay** (Windows). **Canary sync:** upstream `09dbe2c` landed 2026-05-23 ([canary_resync_queue.md](canary_resync_queue.md)). **Edge Tier A+B:** landed 2026-05-23 ([edge_tier_ab_queue.md](edge_tier_ab_queue.md)). **Edge Tier S (partial):** landed 2026-05-23 ([edge_tier_s_queue.md](edge_tier_s_queue.md)); Phoenix HEAD `74fe77ce0`.

## Tier 0 — PC-only differential CI (DONE in canary fork)

Status: **executed end-to-end** on a local MSVC build of `xenia-phoenix-src` (2026-05-15): guest PPC tests green, vmx128-fuzz green at 100k/opcode × 18 opcodes, patch categorizer run on fixture TOMLs, stub-hit JSONL smoke, GPU replay script producing `last_report.json` (synthetic `.xtr` corpus under `tests/gpu_traces/`; strict RTV-vs-ROV gate with `--cross-path d3d12`).

| MVP | Output | Location (in canary fork) |
|-----|--------|---------------------------|
| MVP0 | Reproducible build | `docs/TIER0_BUILD.md`, `tools/tier0/bootstrap.ps1` |
| MVP1 | VMX128 fuzzer + harness | `src/xenia/cpu/testing/guest_ppc_test_util.{h,cc}`, `tools/vmx128_fuzz/*` (registry + families + `main.cc`) |
| MVP2 | GPU trace replay CI | `tools/gpu_replay_ci/run.py`, `gen_phase12_fixture_xtr.py`, `tests/gpu_traces/{README,CORPUS}.md` |
| MVP3 | Patch-debt dashboard | `tools/tier0/categorize_patches.py`, `docs/patch_debt_dashboard.md` |
| MVP4 | Kernel stub-trace JSONL | `src/xenia/kernel/util/stub_trace.{h,cc}` + cvar `kernel_stub_hit_log` |
| PR | Template + workflow | `docs/TIER0_PR_TEMPLATE.md`, `.github/workflows/tier0-windows.yml` |

**Tier 0 exit criteria:** all five MVPs produce green output on a clean build of the fork; CI workflow runs without failures.

---

## Tier 1 — native 360 accuracy

Tier 1 is split into **Tier 1-PC** (differential gates, inventories, instrumentation — no gameplay) and **Tier 1-Gameplay** (smoke captures, patch retirement, retail traces, new-title boot). Gameplay checklist: [61_tier1_gameplay_gate.md](61_tier1_gameplay_gate.md).

### Tier 1-PC — **COMPLETE (x64 Windows, 2026-05-16)**

| Mission item | PC status |
|--------------|-----------|
| vmx128-fuzz ≥1M, zero divergences | **GREEN** — `run_vmx128_full_sweep.ps1` exit 0; PR CI: 50k via `tier0-differential`. Report: `docs/vmx128_fuzz_report.json`. |
| GPU replay zero RTV/ROV drift | **Closed (synthetic)** — retail `.xtr` = gameplay §4 |
| Empty stub JSONL on smoke set | **Gameplay** — infra + CI aggregate done; drain per title in [61_tier1_gameplay_gate.md](61_tier1_gameplay_gate.md) §2 |
| Patch debt C+D = 0 | **Gameplay** — inventory + `list_smoke_patches.py`; retire smoke-linked C/D in §3 |
| New title boots without new patch | **Gameplay** — [61_tier1_gameplay_gate.md](61_tier1_gameplay_gate.md) §5 |

**Handoff:** Pre-gameplay closure landed (CI Tier 0 on PR, telemetry playbook, docs synced). **Enter Tier 1-Gameplay.**

### Tier 1-Gameplay — **IN PROGRESS (NEXT)**

See [61_tier1_gameplay_gate.md](61_tier1_gameplay_gate.md): fill smoke roster (≥5 titles), 5-min captures, C/D patch retirement, retail GPU traces, new-title boot.

---

Tier 1 phases (detail):

Tier 1 is broken into four phases. Each phase has a specific, measurable accuracy goal.

### Phase 1.1 — VMX128 saturation (CPU) — **CLOSED (2026-05-15)**

**Goal:** zero JIT-vs-reference divergences across registered VMX/VMX128 opcodes in `vmx128-fuzz`.

**Status:** **CLOSED (Session 20)** — 1M×2 sign-off green; oracle = `RefPpcVmx*` + `vmx128_pin_table` for sparse FP edges; `vrsqrtefp*` via scalar invoke ref. Gameplay gate: [61_tier1_gameplay_gate.md](61_tier1_gameplay_gate.md).

**Exit criteria:** 1M random samples per opcode, zero divergences _(met via `run_vmx128_full_sweep.ps1`)_.

### Phase 1.1.1 — VMX128 / FMA bit-exactness + stragglers — **CLOSED (2026-05-15)**

- FMA-aware fuzz refs + strict compare; `X64Emitter::HasFMA()`; second RM sweep in `run_vmx128_full_sweep.ps1`.
- `vspltisw128` full imm matrix + `vrlimi128` deterministic matrix prefix in fuzz.
- **NEXT:** Phase **1.3** — kernel-shim coverage (see below).

### Phase 1.2 — GPU EDRAM resolves (GPU) — **CLOSED (2026-05-16)**

**Goal:** RTV and ROV paths produce identical hashed output for the curated trace corpus, and match documented Xenos resolve semantics.

**What landed**

1. **Corpus:** Ten committed synthetic `.xtr` fixtures (`phoenix_slot_*_min.xtr`) + [CORPUS.md](../../xenia-phoenix-src/tests/gpu_traces/CORPUS.md); [gen_phase12_fixture_xtr.py](../../xenia-phoenix-src/tools/gpu_replay_ci/gen_phase12_fixture_xtr.py); [validate_traces.py](../../xenia-phoenix-src/tools/gpu_replay_ci/validate_traces.py).
2. **Harness:** [run.py](../../xenia-phoenix-src/tools/gpu_replay_ci/run.py) strict RTV/ROV gate, `--allow-rtv-rov-drift`, `--format-validate-only`, matched-failure pairing; [run_gpu_replay.ps1](../../xenia-phoenix-src/tools/tier0/run_gpu_replay.ps1); green [last_report.json](../../xenia-phoenix-src/tests/gpu_traces/golden/last_report.json) (zero hash drift on corpus).
3. **CI:** `tier0-windows.yml` runs format validation on traces.
4. **RE:** [re/20_xenos_gpu.md](../re/20_xenos_gpu.md) updated.

**1.2 stretch (ongoing):** legal retail captures per CORPUS matrix; trace-backed `d3d12_render_target_cache.cc` fixes when drift appears; resolve TODO count toward ≤3. **2026-05-23:** Tier A GPU trace button (Edge `96effa1eb` / Phoenix `ba4616de2`); Vulkan `depth_float24_convert_in_pixel_shader` cvar available (default **off**, `466804473`) — opt-in stretch tool for retail capture drift diagnosis when D24FS8 + z_enable; see [edge_tier_s_queue.md](edge_tier_s_queue.md).

**BF2 (`454107DB`):** **parked** — [bf2_gpu_roadmap.md](bf2_gpu_roadmap.md) § Parked. Active Tier 1-Gameplay: smoke roster + [edge_port_queue.md](edge_port_queue.md) + C-patch backlog ([62_gameplay_execution.md](62_gameplay_execution.md)).

### Phase 1.3 — Kernel-shim coverage (kernel) — **PC complete / gameplay remaining**

**Goal:** drain the stub-trace JSONL on the curated smoke-title set.

**Tier 1-PC (2026-05-16)**

1. **Central logging:** every `kStub` export trampoline calls `LogKernelStubHitGuest` when `--kernel_stub_hit_log` is set (`shim_utils.h`).
2. **Inventory:** [kernel_stub_inventory.json](../../xenia-phoenix-src/docs/kernel_stub_inventory.json) (192 kStub exports).
3. **Safe shims:** `X_MEM_RESET`, `NtDeviceIoControlFile` IOCTL paths, `NtQueryFullAttributesFile` root-relative resolve, `ExSetXConfigSetting` user video/audio/language, IOCTL buffer checks without `assert_always`.
4. [aggregate_stub_hits.py](../../xenia-phoenix-src/tools/tier0/aggregate_stub_hits.py), fixture JSONL, CI aggregate step.

**Tier 1-Gameplay (exit criteria)**

1. Fill [60_smoke_titles.md](60_smoke_titles.md); capture 5-min JSONL per title.
2. Implement top-N shims from aggregates; retire category-C patches.
3. Empty stub JSONL during 5-min play per title.

### Phase 1.4 — XMA2 audio + unified smoke telemetry — **CLOSED (2026-05-16)**

**Goal:** XMA2 decode gated by offline fixtures; runtime audio telemetry for unified smoke capture.

**What landed**

1. **RE:** [re/30_xma2_audio.md](../re/30_xma2_audio.md) packet layout + oracle tiers.
2. **Fixtures:** `tests/xma2_packets/` + [gen_fixture_packets.py](../../xenia-phoenix-src/tools/xma2_diff/gen_fixture_packets.py).
3. **Tool:** [xma2-diff](../../xenia-phoenix-src/tools/xma2_diff/) + [run_xma2_diff.ps1](../../xenia-phoenix-src/tools/tier0/run_xma2_diff.ps1); CI step in `tier0-windows.yml`.
4. **Runtime:** `apu_trace`, `--apu_xma_divergence_log`, `--apu_pcm_hash_log`; hooks in `xma_context_new`, `xboxkrnl_audio_xma`, `xaudio2_audio_driver`.
5. **Smoke:** [60_smoke_titles.md](60_smoke_titles.md) audio columns, [run_smoke_capture.ps1](../../xenia-phoenix-src/tools/tier0/run_smoke_capture.ps1), aggregate/compare scripts.
6. **Fixes:** `GetPacketNumber` no longer `assert_always` on header offsets; divergence logging on decode/kernel buffer errors.

**Stretch (smoke-driven):** fill smoke roster; zero XMA JSONL + stable 30s PCM hash per title; golden PCM from legal 360 captures (Q5).

### Tier 1 exit criteria (the "Tier 1 minimally achieved" bar)

See `00_mission.md`. Recap:
- Fuzz: zero divergences.
- GPU traces: zero hash drift.
- Stub log: empty on smoke titles.
- Debt dashboard: zero category C+D patches.
- A new untested title boots without a new patch.

---

## Tier A — HDMI capture-card ground truth (LATER)

Only justified once Tier 1 plateaus.

**Idea:** capture HDMI from a stock retail 360 running a title; capture HDMI from Phoenix running the same title via a synthetic input stream; diff frames.

- Requires a 360 (user has one) and a capture card.
- No console mods needed.
- Test inputs: TR1 (title screen idle), title-screen splash sequences, replays.
- Risk: input determinism. Use scripted controllers (Brook adapter on the 360; Xenia hostile-input replay on PC).

---

## Tier B — Modded console capture (DEFER)

Hardware traces (RGH, JTAG, etc). Out of scope until Tier A is exhausted.

## Tier C — Devkit / PIX-for-Xbox-360 (DEFER)

Effectively unobtainable in 2026. Note for completeness.

---

## Platform phases (post–Tier-1-PC)

Cross-platform work uses **global phase numbers**. **Phase 2 = Linux only.** Phase 3 = Android. Phase 4 = ARM64. (Do not reuse these numbers for Linux sub-work.)

Inventory: [linux_compat_gap_analysis.md](linux_compat_gap_analysis.md). Machine state: [linux_status.md](../../xenia-phoenix-src/docs/linux_status.md). Runbook: [../dev/linux_compat_runbook.md](../dev/linux_compat_runbook.md).

### Phase 2 — Linux (x86_64)

**Policy:** Automated gates (build + `xenia-cpu-tests` + smoke + AppImage help) on every PR. Sub-phases **2.0–2.7** are all Linux. Manual runtime/QA does not block Tier 1 gameplay on Windows.

| Sub-phase | Goal | Status |
|-----------|------|--------|
| **2.0** | Gap analysis | [x] |
| **2.1** | AI brain + INDEX aligned with `xenia-phoenix-src` | [x] |
| **2.2** | Release build green (Docker + CI) | [x] 2026-05-16; CI reconfirmed 2026-05-23 ([run 26341449305](https://github.com/disturbedkh/xenia-phoenix/actions/runs/26341449305) — x86 Release/Debug/Checked) |
| **2.3** | Build matrix (Debug/Checked), `.gitattributes`, CI `doctor`, optional [Dockerfile](../../xenia-phoenix-src/tools/docker/Dockerfile) | [x] |
| **2.4** | Automated verify: [linux-verify.sh](../../xenia-phoenix-src/tools/docker/linux-verify.sh), cpu-tests, AppImage smoke | [x] |
| **2.5** | Runtime sign-off — [linux_runtime_checklist.md](../../xenia-phoenix-src/docs/linux_runtime_checklist.md) (GTK, Vulkan, audio, HID) | [ ] manual; **kernel prep landed** 2026-05-23 — Edge Tier B2 (`ace49597b`, `0672a6872`): POSIX XThread release + timer APC without guest TLS ([edge_tier_ab_queue.md](edge_tier_ab_queue.md)) |
| **2.6** | Smoke-title QA — [60_smoke_titles.md](60_smoke_titles.md) Linux column; stub parity vs Windows | [ ] manual |
| **2.7** | Packaging — [linux_packaging.md](../../xenia-phoenix-src/docs/linux_packaging.md); AppImage on clean Ubuntu VM | [ ] partial (CI smoke [x]) |

### Phase 3 — Android

**Scope:** Android host port only (not Linux desktop, not Phase 4 ARM64 Windows). Inventory: [android_compat_gap_analysis.md](android_compat_gap_analysis.md). Roadmap: [android_compat_roadmap.md](android_compat_roadmap.md). Machine state: [android_status.md](../../xenia-phoenix-src/docs/android_status.md).

**Policy:** Does not block Tier 1 on Windows until **Build 3.x** (green `libxenia-app.so` arm64 Release) is claimed.

| Sub-phase | Goal | Status |
|-----------|------|--------|
| **3.0** | Gap analysis + build matrix | [x] 2026-05-16 |
| **3.1** | AI brain + CMake `ANDROID` + `xe_platform_sources` `*_android` | [x] 2026-05-16 |
| **Build 3.x** | Gradle CMake, `libxenia-app.so`, demos in library | [x] green Docker/NDK r26c 2026-05-16 |
| **Product 3.x** | `EmulatorActivity`, SAF, scoped storage | [x] |
| **Runtime 3.x** | `hid/android`, `apu/android`, file picker, JIT on device | [x] code; JIT manual pending |
| **QA 3.x** | Trace viewer + window demo + one game on arm64 hardware | [ ] manual |
| **Packaging 3.x** | `Android_arm64.yml`, Orchestrator | [x] workflow + Orchestrator; APK release TBD |

### Phase 4 — ARM64

**Scope:** Host ARM64 desktop (Windows WoA **first**). Distinct from Phase 2 x86_64 Linux and Phase 3 Android. Inventory: [arm64_compat_gap_analysis.md](arm64_compat_gap_analysis.md). Roadmap: [arm64_compat_roadmap.md](arm64_compat_roadmap.md). Runbook: [../dev/arm64_compat_runbook.md](../dev/arm64_compat_runbook.md). Machine state: [arm64_status.md](../../xenia-phoenix-src/docs/arm64_status.md).

| Sub-phase | Goal | Status |
|-----------|------|--------|
| **4.0** | Gap analysis (JIT, third_party, CI) | [x] 2026-05-16 |
| **4.1** | Windows ARM64 build + CI (`build-win_arm64.yml`) | [x] first green CI 2026-05-23 |
| **4.2** | `xenia-cpu-tests` + vmx128-fuzz on WoA | [ ] |
| **4.3** | Runtime smoke + [60_smoke_titles.md](60_smoke_titles.md) WoA column | [ ] manual |
| **4.4a** | macOS Apple Silicon (Cocoa + MoltenVK + CI) | [x] landed; CI smoke pending |
| **4.4b** | Linux aarch64 CI (`ubuntu-24.04-arm`) | [x] first green CI 2026-05-23 ([run 26341449305](https://github.com/disturbedkh/xenia-phoenix/actions/runs/26341449305)) |

---

## Working order summary

```
[NOW] Tier 1-Gameplay (Windows)            roster + captures + patch retirement ([62_gameplay_execution.md](62_gameplay_execution.md))
[DONE] Tier 1-PC (x64)                     vmx128 1M + tier0-differential on PR
[PARALLEL] Phase 2.5–2.7 Linux (manual)    runtime checklist, smoke titles, AppImage VM
[PARALLEL] Phase 2.x / tier0-differential  PR CI differential gates
[PARALLEL] Phase 3 Android                 Build 3.x green; device QA manual
[PARALLEL] Phase 4.2–4.3                   cpu-tests + WoA smoke (manual)
[DONE] Phase 4.4b Linux aarch64 CI (2026-05-23)
[PARALLEL] Phase 4.4a macOS CI smoke
[LATER]  Tier A capture-card ground truth
```
