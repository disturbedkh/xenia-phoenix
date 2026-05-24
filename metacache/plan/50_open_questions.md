# Open questions / parking lot

Unresolved decisions. When picking up a session, scan this first.

## Q1. Does Xenia expose its PPC interpreter as a usable oracle?

Currently `vmx128-fuzz` uses scalar C++ as the oracle for VMX opcodes. If we can wire the in-tree PPC interpreter as a second backend in `TestGuestPpcBlock`, we get interpreter-vs-JIT diffing for free.

- File to investigate: `src/xenia/cpu/ppc/ppc_emit_*.cc` and `src/xenia/cpu/backend/*` for whether interpreter mode is selectable at runtime.
- Decision pending.

## Q2. Trace corpus licensing

`tests/gpu_traces/` is currently empty + `.gitkeep`-only. Where do traces come from?

- Option A: keep them out of git; CI fetches from a private artifact bucket.
- Option B: traces from public homebrew titles only.
- Option C: traces are user-supplied; CI runs only when traces are present locally.

Phoenix default: **C** for now. Revisit when CI runs end-to-end.

## Q3. ROV path: maintain or sunset?

`docs/TIER0_README.md` documents the RTV-vs-ROV split (`render_target_path_d3d12`). ROV is more accurate but slower and buggier. Question: do we keep both forever, or commit to one once Phoenix stabilizes?

Defer until Phase 1.2 produces hash-stable corpus.

## Q4. AArch64 backend coverage — **RESOLVED (2026-05-16)**

**Decision:** Add Windows ARM64 cross-build to CI now that Tier 1-PC on x64 is closed. Linux aarch64 CI remains Phase 4.4 (deferred).

**Landed:** `build-win_arm64.yml`, Orchestrator `build-windows-arm64`, `arm64-verify.ps1`, [arm64_compat_gap_analysis.md](arm64_compat_gap_analysis.md). Tier 1 gameplay gates stay x64-first until WoA smoke column is green.

## Q5. XMA2 oracle availability

Where does the XMA2 reference decoder come from? Microsoft's encoder/decoder ships in the Xbox 360 SDK (not redistributable). Public alternatives:

- `libxma2` clones (research-quality, may not be bit-accurate).
- Reference papers + WMA Pro relationship.

If no usable oracle, fallback: differential test against a known-good full-game audio capture. (Quality-of-output rather than bit-exact.)

## Q6. Patch retirement order — **RESOLVED (2026-05-16)**

**Decision:** (1) **Smoke-linked category C** patches from `list_smoke_patches.py` first; (2) **cluster** remaining hits by `(module, export)` from stub aggregates after 5-min captures; (3) full tree 988+80 is stretch, not a gameplay blocker.

Tool: `tools/tier0/list_smoke_patches.py --title-id …` against `docs/patch_debt_dashboard_data.json`.

## Q7. Phoenix branding

Phoenix the fork: do we re-brand the binary name (`xenia-phoenix.exe`), or keep `xenia.exe` and only change the about box? Pirate-distribution risk vs. user confusion.

Decision: keep `xenia_canary.exe` for now. Re-branding is a Tier 1 exit cosmetic.

## Q8. vmx128-fuzz 50k full-registry AV (OQ-vmx128-50k-av)

**Opened:** 2026-05-23 (Tier S follow-ups triage).

**Symptom:** `vmx128-fuzz.exe --vmx128_fuzz_iters=50000 --vmx128_fuzz_seed=3735928559 --vmx128_fuzz_rm_pass=rn` from `build/bin/Windows/Release` cwd crashes immediately with no opcode output. Reproduces on `canary_experimental` baseline before Tier S follow-ups. Per-opcode filters (`vadd*`, `vupk*`) pass 50k.

**Hypothesis:** Local full-registry run accumulates unwind-table / processor state across opcodes differently than CI's per-opcode isolation or CI runner environment. Not a Tier S GPU/kernel regression.

**Next steps:** Compare CI `tier0-differential.yml` invocation vs local; bisect last opcode if logging can be made crash-safe; confirm whether `TestGuestPpcBlock::ReleaseCompiledGuest()` per opcode is skipped on full-registry path.

**Gate policy:** Per-opcode 50k smoke (`vadd*` + `vupk*`) + CI 50k workflow; full-registry local AV tracked here, not blocking merge.
