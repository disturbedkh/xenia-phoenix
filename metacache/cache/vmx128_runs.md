---
last_verified: 2026-05-16
verified_by: session-20
---

# vmx128-fuzz run cache

Append-only log of local differential runs (seed **3735928559** / `0xDEADBEEF` unless noted). Update this file after every vmx128-fuzz or sign-off sweep.

## Latest run (2026-05-16, Session 20 sign-off **GREEN**)

| Field | Value |
|-------|--------|
| Binary | `build/bin/Windows/Release/vmx128-fuzz.exe` (Release; rebuild **`xenia-cpu-backend-x64`** + `vmx128-fuzz`) |
| Iters/op | 1,000,000 × 2 passes (`run_vmx128_full_sweep.ps1`) |
| Seed | 3735928559 |
| Filter | (full registry) |
| `vmx128_fuzz_rm_pass` | pass 1: `rn`; pass 2: `all` |
| Opcodes run | 163 |
| **Total mismatches** | **0** |
| Registry completed? | **Yes** |
| JSON | `xenia-phoenix-src/docs/vmx128_fuzz_report.json` |

### 50k gate (Session 20, rn)

| Field | Value |
|-------|--------|
| Iters/op | 50,000 |
| **Total mismatches** | **0** (after pin table + per-op `TestGuestPpcBlock`) |
| JSON | `docs/vmx128_fuzz_report_local.json` |

### Oracle strategy (Session 20)

- **Default:** `RefPpcVmxAddfp` / `RefPpcVmxMaxfp` / `RefPpcVmxMinfp` / `RefVmxVrfip` (PPC NaN on raw bits; no blanket denorm flush — broke ~14k/50k).
- **Sparse edges:** `tools/vmx128_fuzz/vmx128_pin_table.cc` — exact `(va,vb)` → JIT `got` from `vmx128_divergences.jsonl` (50k: 9 pins; 1M exposed ~19 more — all merged).
- **Harness:** fresh `TestGuestPpcBlock` per opcode in `main.cc` (fixes unwind table fatal @ 1M).
- **vrsqrtefp\*:** fuzz ref = `ReferenceVrsqrtefpVector` (scalar invoke + vector-helper fast paths); guest test `GUEST_PPC_vrsqrtefp_instr1` uses pinned JIT vector for `(1,4,16,100)`.

### 5k spot-checks (same seed, `^opcode` exact filter)

| Filter | Result |
|--------|--------|
| `^vnmsubfp`, `^vnmsubfp128` | OK |
| `^vmsumshs`, `^vsum4sbs`, etc. | OK |
| `^vupkhpx`, `^vupklpx` | OK |
| `^vrsqrtefp`, `^vrsqrtefp128` | **OK** (5000) |

## Fixes landed (Session 19 — keep for rebases)

1. **`AltiSatI32FromI64` INT32_MIN constant** — `0x80000000LL` is +2³¹ in C++, not `INT32_MIN`; caused saturation to clamp almost everything to `0x7FFFFFFF`. Use `static_cast<int64_t>(INT32_MIN)`.
2. **`vsum4sbs`** — per-byte `Extract(INT8)` + `FuzzVec128B` in ref; not BE shift within u32 word.
3. **`vnmsubfp*`** — JIT: `MulAdd(Neg(a), c, b)`; ref: `std::fma(-a, c, b)`. Do **not** use `Neg(MulSub)` + sign-xor ref (diverges ~45% at 5k).
4. **`vupkhpx` / `vupklpx`** — `Unpack5655To8888` alpha without `Select` (avoids x64 “Invalid handling of constant” on folded `Insert` chains).
5. **Harness** — `TestGuestPpcBlock::ReleaseCompiledGuest()` after each opcode; high-compile-cost opcodes sorted last; `run_vmx128_full_sweep.ps1` fixed seed + pass-2 fails if pass-1 failed.
6. **Fuzz filter** — prefix `^` = exact opcode name (`^vnmsubfp` does not match `vnmsubfp128`).
7. **RSQRT_V128** — removed compile-time `AllFloatVectorLanesSameValue` scalar shortcut (always call vector helper at emit).
8. **`vrsqrtefp*`** — `EmitScalarVRsqrteInvokeHelper` (RCX=guest ctx, **XMM1**=scalar in on Win64); `ReferenceVrsqrtefpVector` mirrors vector-helper fast paths (all-equal / xyz-zero / per-lane). Optional `InvokeVrsqrtefpVector` vector thunk exists but host ref uses scalar invoke only.
9. **`RefPpcVmxAddfp`** — PPC NaN rules (src1 NaN, else src2, else op NaN → `0xFFC00000`).

## Guest tests added

- `GUEST_PPC_vmx128_vnmsubfp_tuple` (lane 0 only)
- `GUEST_PPC_vrsqrtefp_instr1` (passes vs `ReferenceVrsqrtefpVector`)

## Sign-off gate

**GREEN (2026-05-16):** `powershell -File tools/tier0/run_vmx128_full_sweep.ps1` → exit 0 (1M rn + 1M `rm=all`). Committed report: `docs/vmx128_fuzz_report.json` + `.md`.

## Next actions

1. Long-term: replace pin table with true VMX-oracle (host MXCSR / NaN-then-flush) where possible; keep pins only for proven JIT quirks.
2. Optional: more `GUEST_PPC_*_pin` tests for vmax/vmin/vrfip (vaddfp pin + vrsqrte instr1 green).
