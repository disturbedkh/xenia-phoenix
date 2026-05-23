# Xenon CPU + VMX128

## Core facts

- **3 x PowerPC 64-bit cores** (in-order, 2-way SMT each => 6 hardware threads).
- **3.2 GHz**, shared 1MB L2, no L3. Per-core 32 KB I + 32 KB D L1.
- **In-order issue.** Branch mispredicts are expensive; many game patches were written assuming this.
- **VMX128**: 128-entry vector register file (vs. 32 on PowerPC), extended opcode encodings, dot-product ops, packed types.
- **PPC base ISA + Cell-style additions**: not identical to Cell PPE but architecturally similar (in-order, dual-issue, SMT2).

## Things Xenia handles

| Feature | Xenia handling | File |
|---------|----------------|------|
| PPC integer/FP base ops | HIR translation -> x64/aarch64 backend | `src/xenia/cpu/ppc/ppc_emit_*.cc` |
| VMX/VMX128 vector ops | HIR vector ops -> SSE/AVX/NEON | `src/xenia/cpu/ppc/ppc_emit_vmx.cc` |
| MSR / SPR | mostly modeled as guest state, some shimmed | `src/xenia/cpu/ppc/ppc_context.h` |
| dcbz / dcbf / icbi | partial (`dcbz` matters for game writes that assume zeroed cache lines) | various |
| HV / hypervisor calls | not modeled; guest runs in pseudo-supervisor | n/a |

## Known accuracy gaps (Tier 1 phase 1.1 targets)

These were observed as `// HACK` in `src/xenia/cpu/backend/a64/a64_seq_vector.cc` (17 occurrences as of fork) and elsewhere:

- Saturating arithmetic edge cases on `vadd*s` / `vsub*s` (signed overflow boundary).
- `vperm` / `vsldoi` shuffle byte-order subtleties.
- Floating-point denormal handling (Xenon has flush-to-zero in some modes).
- `vmsum*` accumulator widening.
- `vmaddfp` / `vnmsubfp` rounding mode; the 360 sometimes uses `RN` (round-to-nearest) while x64 default is the same — but flag preservation differs.

## VMX128 opcode encoding

VMX128 reuses base PPC vector encoding space and adds:

- `VX128`, `VX128_1`, `VX128_2`, `VX128_3`, `VX128_4`, `VX128_5`, `VX128_P`, `VX128_R`, `VX128_FX` forms.
- Extended VR field: high bit comes from a different position in the encoded word.

Search Xenia for `kVX128` constants and `Disasm_VX128`.

## Reference resources

- `Xbox 360 CPU Documentation` (community-aggregated; google "xenon cpu ppc reference manual").
- `IBM PowerPC Architecture Book I/II/III` (free PDFs).
- Free60 wiki: https://free60.org/System-Software/CPU/
- Xenia source `src/xenia/cpu/ppc/instructions/*.cc` is itself a reasonable reference, just verify against the architecture book.

## Phoenix oracle strategy

For Tier 1.1 the oracle is a **scalar C++ reference** in test code (see `tools/vmx128_fuzz/main.cc`), not the in-tree interpreter. Reasons:

1. Direct, easy to read.
2. Guarantees orthogonality from the JIT (no shared bugs).
3. Doesn't depend on whether Xenia's interpreter mode is exposed (see `plan/50_open_questions.md` Q1).

Long-term: if the interpreter is wired up as a runtime backend, add it as a third oracle for triangulation.
