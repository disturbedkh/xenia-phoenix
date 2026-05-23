---
area: APU
last_verified: 2026-05-17
---

# Universal: APU / XMA / PCM

Parent RE: [30_xma2_audio.md](../../re/30_xma2_audio.md).

## U-APU-001: PCM hash drift as regression sentinel

**Status:** confirmed (harness)

`apu_pcm_hash_log` JSONL (~1 Hz) detects audio path regressions without bit-exact XMA oracle.

**Games:** All smoke titles with `telemetry/{tid}_pcm.jsonl`.

---

*Add `U-APU-nnn` when XMA divergence patterns repeat across titles.*
