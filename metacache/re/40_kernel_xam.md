# Kernel and XAM

## Core facts

The 360 user-mode game runs against three "kernel" surfaces:

- **xboxkrnl.exe** — Win32-style kernel (process, thread, memory, file, sync, IO completion ports). Most APIs map roughly to NT.
- **xam.xex** — game-facing UI / system services (gamercard, achievements, sign-in, content browse, save UI, voice). Behaves like a higher-level shell.
- **xbdm.xex** — debug-monitor (only present on devkit; Phoenix mostly ignores).

## Xenia's modeling

`src/xenia/kernel/`:

```
xboxkrnl/   <- xboxkrnl.exe shims
xam/        <- xam.xex shims
xbdm/       <- xbdm.xex shims (minimal)
util/       <- shared helpers (Phoenix added stub_trace.{h,cc} here)
```

Each shim file groups related functions:

- `xboxkrnl_memory.cc` — `NtAllocateVirtualMemory`, `MmAllocate*`, ...
- `xboxkrnl_threading.cc` — threads, events, semaphores, critical sections.
- `xboxkrnl_io.cc` — file I/O, completion ports, devices.
- `xboxkrnl_misc.cc`, `xboxkrnl_modules.cc`, `xboxkrnl_strings.cc` — etc.

## Stub mechanism

Each export is registered with `DECLARE_XBOXKRNL_EXPORT(...)` macro, which wires the guest call to a C++ entry function. Unimplemented exports either:

- log `XELOGE("X not implemented")` and return a default value, or
- terminate with `assert_unhandled_case`.

Phoenix Tier 0 added `LogKernelStubHit` (`src/xenia/kernel/util/stub_trace.{h,cc}`) which writes a structured JSONL row when the cvar `--kernel_stub_hit_log=path.jsonl` is set.

## What Phoenix logs per stub hit

```json
{
  "module": "xboxkrnl",
  "export": "NtAllocateVirtualMemory_X_MEM_RESET",
  "detail": "optional",
  "title_id": "4D5307D1",
  "lr": "82001234"
}
```

`title_id` and `lr` are emitted when using `LogKernelStubHitGuest` from a guest export (or passed explicitly). Use `tools/tier0/aggregate_stub_hits.py` on capture JSONL; roster template: [plan/60_smoke_titles.md](../plan/60_smoke_titles.md).

**Implemented (Tier 1-PC):**

- `X_MEM_RESET` in `NtAllocateVirtualMemory` zeros committed guest regions.
- **Central stub telemetry:** `RegisterExport` trampolines in `shim_utils.h` call `LogKernelStubHitGuest` for every export tagged `kStub` when `--kernel_stub_hit_log` is set (static inventory: `docs/kernel_stub_inventory.json`).
- `NtDeviceIoControlFile` — disk geometry/partition IOCTLs; unknown IOCTL → `STATUS_INVALID_DEVICE_REQUEST` (no `assert_always`). Export tagged **implemented** (was stub).
- `NtCancelIoFile`, `NtFlushBuffersFile`, `FscGetCacheElementCount`, `FscSetCacheElementCount`, `IoCreateDevice`, `IoDeleteDevice` — safe success paths; exports tagged **implemented** (Tier 1-PC).
- `NtQueryFullAttributesFile` — resolves paths relative to `root_directory` file handle.
- `ExSetXConfigSetting` — persists `XCONFIG_USER_VIDEO_FLAGS`, `XCONFIG_USER_AUDIO_FLAGS`, `XCONFIG_USER_LANGUAGE` into cvars.
- **CPU (Tier 1-PC):** `vmaddfp128` / `vnmsubfp128` emitters use explicit `MulAdd`/`MulSub` paths when VD aliases VC (see `ppc_emit_altivec.cc`).

**Implemented (Tier 1-Gameplay — BF2 `454107DB` stub drain, 2026-05-16):**

- `XAudioGetVoiceCategoryVolumeChangeMask` — no volume-change mask; removed host `NanoSleep`; **implemented**.
- `VdGetSystemCommandBuffer` / `VdRetrainEDRAM*` — **behavior unchanged** (Canary `0xBEEF*` + `return 0`); promoted to **implemented** only to stop stub JSONL spam. Experimental template/retrain=1 reverted (BF2 boot hang).
- Promoted to **implemented** (existing bodies): `VdQueryVideoMode`, `VdQueryVideoFlags`, `VdQueryRealVideoMode`, `VdGetCurrentDisplayInformation`, `VdGetCurrentDisplayGamma`, `VdIsHSIOTrainingSucceeded`, `VdInitializeEngines`, `VdGetGraphicsAsicID`, `VdSetDisplayMode`, `KiApcNormalRoutineNop`.
- XAM (BF2 boot): `XNotifyPositionUI`, `XamContentGetDeviceState`, `XamUserCheckPrivilege`.
- XAudio voice categories (2026-05-16): `XAudioGet/SetVoiceCategoryVolume`, `XAudioGetVoiceCategoryVolumeChangeMask` — 32-slot store + change mask (was stub/no-op Set).

## Coverage estimate

- xboxkrnl ordinals on retail: ~700+. Xenia covers a meaningful subset (most of process/thread/memory/file/sync). Many edge ordinals are stubs.
- xam ordinals: ~500+. UI surfaces (gamercard, NUI, party chat, achievements) are partially modeled with the canary XAM UI.

## Bug clusters historically

- Memory: `MmAllocatePhysicalMemoryEx` flag bits, `NtAllocateVirtualMemory` allocation type semantics, page-protection edge cases. (See `XELOGE("X_MEM_RESET not implemented")` site.)
- Threading: priority bands, SMT-aware affinity, fiber semantics.
- Filesystem: STFS package mounting, `XContentCreate*`, profile mounts.
- Networking: title-server APIs (Xenia mostly stubs; live services are gone anyway).
- Achievements / gamercard: many code paths return "no profile" defaults.

## Phoenix RE backlog for kernel

1. Audit every `XELOGE` / `XELOGW` *not implemented* site, attach `LogKernelStubHit` (partial: `kernel_module`, `xboxkrnl_crypt`, `xboxkrnl_memory`).
2. Build a coverage table: ordinal -> implemented / stub / hit-frequency.
3. Document the `XContent` package mounting code path end-to-end.
4. Document profile-load behavior (real vs. mock profile).

## References

- Free60 wiki: https://free60.org/System-Software/Modules/
- Xbox 360 SDK API docs (leaked): use as **read-only** RE reference; do not paste into source.
- Xenia source itself is the most up-to-date map of which APIs are wired.
