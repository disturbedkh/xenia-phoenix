# Patch debt triage (PC inventory)

Source: `docs/patch_debt_dashboard_data.json`

## Counts

```json
{
  "B": 945,
  "C": 598,
  "A": 83,
  "D": 61
}
```

## Top C/D titles (heuristic subsystem)

| Title ID | C+D rows | Subsystem guess | Sample patch |
|----------|----------|-----------------|--------------|
| 5454082B | 41 | gpu | Disable Sun Flare |
| 544307D1 | 25 | unknown | Unlock All Menus |
| 4D5308BC | 23 | unknown | Disable Achievements |
| 4541080F | 19 | unknown | -dvd |
| 545107FC | 19 | unknown | PS3 Button Prompts |
| 58410955 | 17 | gpu | HD Shadows |
| 545107D1 | 16 | unknown | Tree LODs |
| 4D5307ED | 15 | unknown | Fix crashing on start-up |
| 415607E1 | 13 | unknown | Remove Grass |
| 454108CE | 12 | unknown | Black Shading Fix |
| 4D530A26 | 12 | gpu | Disable Lens Flares |
| 4D5307D5 | 11 | unknown | Black Shading Fix |
| 545107F8 | 11 | unknown | Tree LODs |
| 545408A7 | 11 | unknown | GTA IV Wanted System |
| 4D53082D | 10 | unknown | Black Shading Fix |
| 4D5308AB | 10 | gpu | Disable Lens Flares |
| 4D5307DC | 8 | gpu | Global Lights Rendering |
| 4D5307E8 | 8 | unknown | Black Shading Fix |
| 53450812 | 8 | unknown | Disable Color Adjustment |
| 58411403 | 8 | unknown | Corrupted Graphics Fix |
| 545407F8 | 7 | gpu | Disable Imposter Shadows - Performance Mode |
| 565707D0 | 7 | gpu | Disable Lens Flares |
| 425607ED | 6 | gpu | Disable Lens Flares |
| 45410850 | 6 | gpu | Dynamic Lighting & Shadows Shading Fix |
| 454108EF | 6 | gpu | Disable Lens Flares |

## Likely false-positive category C (~107 rows)

Cosmetic/QoL keywords — verify on hardware before gameplay retirement:

- `button prompt`
- `ps3 button`
- `unlock fps`
- `show fps`
- `widescreen`
- `fov`
- `brightness`
- `disable lens`
- `disable motion blur`
- `disable dof`
- `hd shadows`
- `anisotropic`

## Gameplay phase

Per title: disable patch → smoke capture (stub/XMA logs) → fix → delete.
See `Xenia-Phoenix/metacache/plan/61_tier1_gameplay_gate.md`.
