# Known pitfalls

Things that bit us. Future agents: do not repeat these.

## P-001: `cmake` not on PATH — RESOLVED 2026-05-15

**Was:** cmake not installed; VS not installed.

**Fix applied:** installed cmake 4.3.2 standalone via `winget install Kitware.CMake`. cmake is now at `C:\Program Files\CMake\bin\cmake.exe` and on the system PATH permanently.

VS 2022 Community installed via winget with `--add Microsoft.VisualStudio.Workload.NativeDesktop`. If VS is ever missing or needs a repair:

```powershell
winget install --id Microsoft.VisualStudio.2022.Community --accept-package-agreements --accept-source-agreements --override "--add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --quiet --norestart"
```

## P-002: PowerShell `&&` chaining

**Symptom:** `The token '&&' is not a valid statement separator in this version`

**Cause:** PowerShell <7 does not support `&&`. Use `;` for unconditional sequencing or `if ($?) { ... }` for conditional.

**Fix:** prefer separate lines or the Shell tool's `working_directory` arg. For commands that **must** be conditional in one shot, use:

```powershell
cmake -S . -B build ; if ($?) { cmake --build build --config Release }
```

PowerShell 7+ supports `&&`/`||` like bash. Phoenix targets PS5/PS7 compatibility, so prefer line-separated.

## P-003: `StrReplace` rejected because old/new identical

**Symptom:** "old_string and new_string are exactly the same"

**Cause:** the file already contains the desired text; the replace is a no-op.

**Fix:** Read the file first; verify what's actually there. Don't blindly replay edits.

## P-004: Empty `tests/gpu_traces/` directory in CI

**Symptom:** `gpu_replay_ci/run.py` reports zero traces and exits 0; CI passes silently with no coverage.

**Cause:** trace corpus is local-only by default (see `plan/50_open_questions.md` Q2).

**Fix:** require `--require-traces` flag in CI to fail when no traces present, or document this as expected.

## P-005: Mixing canary changes with Phoenix-only deviations

**Symptom:** rebases against canary upstream get noisy.

**Fix:** group Phoenix-only commits with `[phoenix]` prefix (or by branch). Keep canary-aligned PRs separate from Phoenix-only PRs.

## P-006: Adding logging without structured schema

**Symptom:** new `XELOGE("...not implemented")` calls don't show up in stub-hit JSONL.

**Fix:** every new "not implemented" log site must also call `LogKernelStubHit`. See `src/xenia/kernel/util/stub_trace.h`.

## P-007: Catch2 macro confusion

**Symptom:** Catch2 tests compile but don't run (no test discovered).

**Cause:** missing `TEST_CASE(...)` or wrong `[tag]`.

**Fix:** verify with `xenia-cpu-tests.exe --list-tests`.

## P-008: Building Checked vs. Release

**Symptom:** Release build tests pass, Checked build crashes immediately.

**Cause:** Checked build enables aggressive asserts that catch undefined behavior. **This is by design.** Treat Checked-only failures as real bugs.

**Fix:** never disable a Checked-only assert without root-cause; that's a Tier 1 bug.

## P-009: `--xenia-build` path on `gpu_replay_ci`

**Symptom:** "trace dump tool not found" from `run.py`.

**Cause:** the script expects the directory containing `xenia-gpu-d3d12-trace-dump.exe`, not the build root.

**Fix:**

```powershell
python .\tools\gpu_replay_ci\run.py --xenia-build .\build\src\xenia\gpu\d3d12\Release
```

## P-010: Forgetting `git submodule update --init --recursive`

**Symptom:** CMake configure errors about missing third_party headers.

**Fix:** always run submodule init before first configure (or use `tools/tier0/bootstrap.ps1`).

## P-011: MSVC + Ninja requires vcvars64 — RESOLVED 2026-05-15

**Symptom:** `Cannot open include file: 'stddef.h'` / `'algorithm'` when building with `cmake --build` or raw `ninja` outside a Developer Command Prompt.

**Cause:** CMake configures fine (finds `cl.exe`), but Ninja does not automatically load the MSVC environment variables (INCLUDE, LIB, PATH extension). Standard library headers are missing.

**Fix:** always wrap build commands inside the vcvars64 shell:
```powershell
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" && cd /d `"$src`" && set VULKAN_SDK=C:\VulkanSDK\1.4.350.0 && python xenia-build.py build"
```

## P-012: Vulkan SDK required for SPIRV shader compilation — RESOLVED 2026-05-15

**Symptom:** `FileNotFoundError: [WinError 2]` when Ninja runs `compile_shader_spirv.py`.

**Cause:** `glslangValidator`, `spirv-opt`, `spirv-dis` are sourced from `%VULKAN_SDK%\Bin`. Without the SDK the SPIRV shader step always fails.

**Fix:**
```powershell
winget install --id KhronosGroup.VulkanSDK --accept-package-agreements --accept-source-agreements --silent
# Then set VULKAN_SDK before building:
$env:VULKAN_SDK = "C:\VulkanSDK\1.4.350.0"
```

## P-013: `PPCContext` is a typedef, cannot forward-declare — RESOLVED 2026-05-15

**Symptom:** `error C2371: 'xe::cpu::ppc::PPCContext': redefinition; different basic types`

**Cause:** `PPCContext` is `typedef struct alignas(64) PPCContext_s { ... } PPCContext;`. A forward declaration `struct PPCContext;` or `class PPCContext;` conflicts with the typedef definition.

**Fix:** include `"xenia/cpu/ppc/ppc_context.h"` directly; do not forward-declare `PPCContext`.

## P-014: Double-opening `namespace xe` leaks enclosing scope — RESOLVED 2026-05-15

**Symptom:** `xe::std::integral_constant` / `xe::std::_Ratio_multiply_sfinae` — std headers fail inside xenia compilation units.

**Cause:** A header opened `namespace xe { namespace cpu { ... }` but forgot the closing `} // namespace xe` before opening `namespace xe { namespace kernel {` again. Everything after the second open is inside `xe::xe`.

**Fix:** always match open/close braces; run the build locally before pushing any new header.

## P-015: `xb build` (Debug) vs Release — RESOLVED 2026-05-15

**Symptom:** `xb build` builds the `Debug` configuration by default; passing `--config Release` to raw ninja is needed for size-optimised builds.

**Fix:** use `python xenia-build.py build` for debug (what CI checks), use `ninja -f build-Release.ninja` for release. The `xenia_canary.exe` debug binary is ~40 MB; release will be significantly smaller.

## P-016: Docker daemon not running — RESOLVED 2026-05-16

**Symptom:** `failed to connect to the docker API at npipe:////./pipe/dockerDesktopLinuxEngine`

**Fix:** Start Docker Desktop; wait for `docker info` to show Server. Use `tools/docker/run-linux-build.ps1` (auto-starts Desktop).

## P-017: Wrong tree / empty submodules — OPEN

**Symptom:** CMake errors, missing `third_party/imgui/imgui.cpp`, empty submodule dirs.

**Cause:** Building `xenia-canary-canary_experimental/` without git, or never running `git submodule update`.

**Fix:** Build only `Xenia-Phoenix/xenia-phoenix-src/`. Docker script runs submodule init inside container.

## P-018: `VULKAN_SDK` / old `spirv-opt` — OPEN

**Symptom:** Shader compile fails on `--canonicalize-ids`; runtime SPIR-V validation fails.

**Fix:** Install LunarG Vulkan SDK; `export VULKAN_SDK=...`. Build has fallback for old spirv-opt; runtime needs `libSPIRV-Tools-shared.so` (see `spirv_tools_context.cc`).

## P-019: CRLF in `tools/docker/*.sh` — RESOLVED 2026-05-16

**Symptom:** `/bin/bash^M: bad interpreter` or script exits immediately in container.

**Fix:** `run-linux-build.ps1` runs `sed -i 's/\r$//'` before execute. Save shell scripts with LF line endings.

## P-020: CRLF in `.gitmodules` breaks Docker submodule init — RESOLVED 2026-05-16

**Symptom:** `error: pathspec 'third_party/foo?' did not match any file(s) known to git` inside Ubuntu container.

**Cause:** `.gitmodules` checked out with CRLF on Windows; `grep -oP` leaves `\r` on paths.

**Fix:** `linux-build.sh` uses `tr -d '\r'` on submodule paths. Prefer `git config core.autocrlf input` or normalize `.gitmodules` to LF.

## P-021: `git submodule update -j` on Ubuntu 24.04 — RESOLVED 2026-05-16

**Symptom:** `usage: git submodule` immediately after submodule sync in Docker.

**Cause:** Debian/Ubuntu `git` 2.43 does not support `-j` on `submodule update` (unlike newer Git for Windows).

**Fix:** Use `git submodule update --init --depth=1` without `-j` in `linux-build.sh` and CI.

## P-022: Stale premake `xenia.wks.Android.mk` on Canary — RESOLVED 2026-05-16 (Phase 3)

**Symptom:** Android Studio Gradle sync fails looking for `build/xenia.wks.Android.mk`.

**Cause:** CMake migration removed root premake; old `android_studio_project` still used `ndkBuild` + premake output.

**Fix:** Phoenix uses `externalNativeBuild { cmake { path '../../../CMakeLists.txt' } }`. Do not restore premake-androidndk unless explicitly maintaining dual build systems.

## P-023: JNI symbols stripped from `xenia-ui` static lib — RESOLVED 2026-05-16

**Symptom:** `UnsatisfiedLinkError` for `Java_jp_xenia_emulator_WindowedAppActivity_*` at runtime.

**Cause:** Linker `--gc-sections` drops unreferenced JNI entry points from static archives.

**Fix:** Link `xenia-app` with `-Wl,--whole-archive` on `xenia-ui` (see `src/xenia/app/CMakeLists.txt`).

## P-024: SDL on Android — avoid

**Symptom:** Broken gamepad keycode mapping if SDL HID is enabled on Android.

**Fix:** Use `hid/android` only; `add_subdirectory(hid/sdl)` is behind `NOT ANDROID`.

## P-025: `AltiSatI32FromI64` and `0x80000000LL`

**Symptom:** `vmsumshs` / `vsum*` JIT always `0x7FFFFFFF`; fuzz 100% fail on sat opcodes.

**Cause:** `LoadConstantInt64(0x80000000LL)` is **+2147483648**, not `INT32_MIN` (`-2147483648`).

**Fix:** `static_cast<int64_t>(INT32_MIN)` and `static_cast<int64_t>(INT32_MAX)` in `ppc_emit_altivec.cc`.

## P-026: `vnmsubfp` ref vs `Neg(MulSub)`

**Symptom:** ~97% fuzz mismatch on `vnmsubfp` despite “correct” `std::fma(-a,c,b)` ref.

**Cause:** JIT does `vfmsub213ps` then **sign-bit XOR**; that is not always equal to arithmetic negation or `fma(-a,c,b)`.

**Fix:** Emitter `MulAdd(Neg(a), c, b)`; ref `std::fma(-a, c, b)` after denorm flush. See `re/10_vmx128.md`.

## P-027: vmx128-fuzz filter substring `vnmsubfp` matches `vnmsubfp128`

**Symptom:** Filter `vnmsubfp` runs both opcodes; mismatch counts look ~2× expected.

**Fix:** Use exact filter `^vnmsubfp` / `^vnmsubfp128` (implemented in `tools/vmx128_fuzz/main.cc`).

## P-028: Stale 1M vmx128 logs vs current build

**Symptom:** Terminal shows `unwind_table_count_ exceeded` at `vslb` or old opcode failure mix.

**Cause:** Pre-Session-19 binary or no `ReleaseCompiledGuest()` between high-JIT families.

**Fix:** Rebuild Release `vmx128-fuzz`; trust [cache/vmx128_runs.md](../cache/vmx128_runs.md) not old terminal captures. Full 50k must finish 163 opcodes before 1M sign-off.

## P-029: vmx128-fuzz shared guest block @ 1M

**Symptom:** `unwind_table_count_ exceeded` mid-registry (~opcode 140+).

**Fix:** Construct a fresh `TestGuestPpcBlock` per opcode in `tools/vmx128_fuzz/main.cc` (Session 20).

## P-030: Blanket denorm flush before FP add ref

**Symptom:** `vaddfp` mismatches jump to ~14k/50k after adding `RefVectorDenormFlush` before add.

**Fix:** NaN checks on **raw** bits first; pins for sparse edges. See `re/10_vmx128.md`.

## P-031: `vmx128_divergences.jsonl` staleness

**Symptom:** Pin rows do not reproduce; 1M finds new vectors not in 50k capture.

**Fix:** `Remove-Item vmx128_divergences.jsonl` before each diagnostic run; use `^opcode` filter; merge first line per opcode only.

## P-032: `InvokeVrsqrtefpVector` from unit tests

**Symptom:** SIGSEGV when calling vector invoke helper from `ReferenceVrsqrtefpVector` in isolation.

**Fix:** Use scalar-invoke fast-path ref for fuzz; pin JIT output in `GUEST_PPC_vrsqrtefp_instr1`.
