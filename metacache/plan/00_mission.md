# Phoenix mission

## North star

Project Phoenix exists to take Xenia Canary from "many games boot, with hand-tuned per-title patches" to **bit-accurate, native Xbox 360 emulation** that does not need per-game patches to work for the average title.

## What we believe

1. **Per-game patches are technical debt, not features.** Every patch in `game-patches/*.patch.toml` is a `memcpy` into guest memory at a fixed address that exists because the emulator could not honestly execute the original instruction stream. Each one represents an emulator bug, an unimplemented kernel API, or a GPU/audio accuracy gap.
2. **Tier 1 is reachable from a PC alone.** A large fraction of remaining bugs (VMX128 corner cases, EDRAM resolves, kernel-shim coverage) are detectable and fixable with **internal-consistency tests** (interpreter vs. JIT, RTV vs. ROV, scalar reference vs. vector implementation). We do not need a 360 console to make most of this progress.
3. **The compatibility list is a lagging indicator.** It rewards "the title boots once on someone's PC" rather than "the title is reproducibly correct." Phoenix instead measures progress with hashes of GPU traces, kernel-stub-hit JSONL, and patch-debt categorization.

## Non-negotiables

- **No piracy enablement.** Tooling that requires content dumps assumes the user legally owns the disc.
- **No kernel-shim regressions.** Any change touching `xboxkrnl/` must keep existing shim contracts; new behavior gates behind a cvar.
- **No GPU regressions on traced titles.** Anything in `tests/gpu_traces/` must keep producing the same image hashes (or update them deliberately).
- **All deviations from canary are documented.** Phoenix tracks its diff vs. canary so we can rebase and so upstream can adopt patches that are good for them too.

## Definition of done for "Tier 1 minimally achieved"

When all five hold:

1. `vmx128-fuzz` runs all VMX128 opcodes for >= 1M random vectors per opcode against a scalar reference and finds zero divergences in CI.
2. `tools/gpu_replay_ci/run.py` reports zero hash drift across the curated trace corpus on both RTV and ROV paths.
3. The kernel stub-hit JSONL log is empty for the curated "smoke title" set (i.e., none of those titles call an unimplemented kernel function during a 5-minute play session).
4. The patch-debt dashboard shows category C and D (workarounds for emulator bugs) at zero. Only categories A and B (legitimate game patches: cheats, language packs) remain.
5. A randomly-selected new title (one we have not tested) boots and reaches gameplay without any new patch entry.

This is the bar. Everything else is a stepping stone.
