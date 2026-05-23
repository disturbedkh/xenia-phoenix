# BF2 Edge playtest notes (`454107DB`)

**Date:** 2026-05-16  
**Platform:** Xenia Edge (user session)  
**Gfx:** Community workaround per [compat #9 kortul](https://github.com/xenia-canary/game-compatibility/issues/9#issuecomment-3997637068) — typically:

- `depth_float24_convert_in_pixel_shader = true`
- `gpu_allow_invalid_upload_range = true`

## Results

| # | Area | Result |
|---|------|--------|
| 1 | Ground flicker (kortul fix) | **Fixed** — GitHub workaround still works on Edge |
| 1b | Secondary shadow after fix | **Still present** — see G-454107DB-004 |
| 2 | Boot video, loading → main menu, mission-load A/V | **Fixed** on Edge |
| 3 | In-mission audio + stability | **Open** — audio lost, then crash into mission (G-454107DB-013) |
| 4 | Rainbow rays from sky | **Open** — random, intermittent; not addressed by kortul cvars (G-454107DB-014) |

## Next capture (if reproducing crash or rays)

On Edge or Phoenix, note:

- Mode: Campaign / Challenges / which mission (`zone_*` from log if available)
- Edge build commit or version string
- Crash log path (if any)
- F4 `.xtr` while sky rays visible or just before crash

Phoenix instrumented path: `bf2_gpu_session.ps1 -Launch` → play → F4 → `-PostOnly`.

Full findings: [454107db.md](../findings/games/454107db.md) G-454107DB-011 through G-454107DB-014.
