# Project Phoenix (Xenia-Phoenix)

**Mission:** an Xbox 360 emulator that aims squarely at **Tier 1 — native, accurate 360 emulation** — and treats the per-game patch ecosystem inherited from canary as a **debt ledger to be paid down**, not a permanent dependency.

Phoenix is the working copy where Xenia Canary will deviate. Upstream stays at:

- `..\xenia-master\` — original Xenia (mostly archival).
- `..\xenia-canary-canary_experimental\` — Xenia Canary `canary_experimental` branch (reference snapshot).
- `.\xenia-phoenix-src\` — canonical build tree (git + submodules).
- `.\` (this folder) — Project Phoenix: metacache + coordination.

## Tier model

| Tier | Description | Phoenix policy |
|------|-------------|----------------|
| **0** | PC-only differential testing, internal-consistency CI, build hygiene | Always green. CI gate. |
| **1** | True/native 360 emulation: bit-accurate VMX128, EDRAM/Xenos resolves, XMA2, kernel coverage | **Primary target.** Every PR moves toward Tier 1. |
| **2** | Per-game compatibility patches (`game-patches`) | Tracked in `metacache/plan/30_debt_ledger.md`. Goal: shrink, not grow. |
| **A** | HDMI capture-card visual/audio ground truth (no console mods) | Optional uplift; deferred until Tier 0/1 saturate. |
| **B** | Modded-console hardware traces / homebrew capture tools | Out of scope until A is exhausted. |
| **C** | Devkit / PIX-for-Xbox-360 | Effectively unobtainable in 2026. |

## Metacache (agent memory)

`metacache/` is the persistent memory surface for Phoenix agents:

- [`metacache/plan/`](metacache/plan/) — strategy, roadmap, debt ledger, gameplay gates.
- [`metacache/re/`](metacache/re/) — Xbox 360 RE notes.
- [`metacache/dev/`](metacache/dev/) — build commands, tests, runbooks.
- [`metacache/agents/`](metacache/agents/) — specialist agent cards + [registry.yaml](metacache/agents/registry.yaml).

Read [`metacache/INDEX.md`](metacache/INDEX.md) or [`metacache/QUICKSTART.md`](metacache/QUICKSTART.md) first.

## Disclaimer

Phoenix is research and educational. It is **not** for enabling piracy. Tooling that requires asset dumps assumes you legally own the relevant disc / console.
