---
area: KRN
last_verified: 2026-05-17
---

# Universal: kernel / XAM

Parent RE: [40_kernel_xam.md](../../re/40_kernel_xam.md).

## U-KRN-001: Stub export spam masks real failures

**Status:** confirmed

Games poll unimplemented exports every frame → huge JSONL → hides new stubs.

**Fix direction:** Promote to `kImplemented` with safe semantics, or implement; central `LogKernelStubHitGuest` when log enabled.

**Games:** BF2 [G-454107DB-001](../games/454107db.md) (resolved).

---

*Add `U-KRN-nnn` sections below as patterns emerge from smoke captures.*
