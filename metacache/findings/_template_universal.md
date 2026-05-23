---
area: GPU          # GPU | KRN | APU | VFS | CPU
last_verified: YYYY-MM-DD
status: open       # open | confirmed | fix-landed | wont-fix
---

# U-<AREA>-<nnn>: <short title>

## Pattern

What fails in translation (not which game).

## Guest behavior (expected)

What the 360 title is doing (packets, EDRAM bases, resolves, etc.).

## Emulator gap

Which Xenia subsystem is wrong (`render_target_cache`, `shared_memory`, …).

## Symptoms (user-visible)

Flicker, stripes, halo, silent audio, …

## Detection

How to spot in logs, probe, or traces without guessing.

## Proof required

- [ ] Synthetic or legal `.xtr` in CORPUS matrix
- [ ] RTV/ROV replay (`run_gpu_replay.ps1`) if GPU
- [ ] Drift log row in [re/20_xenos_gpu.md](../re/20_xenos_gpu.md) when applicable

## Fix direction

Concrete `src/` change (not "enable cvar X forever").

## Games exhibiting (link G-*)

| Game | Finding | Notes |
|------|---------|-------|
| | | |

## References

- RE: 
- External: 
