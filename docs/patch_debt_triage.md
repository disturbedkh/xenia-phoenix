# Patch debt triage (PC inventory)

Source: `docs/patch_debt_dashboard_data.json`

## Counts

```json
{
  "B": 536,
  "C": 988,
  "D": 80,
  "A": 83
}
```

## Top C/D titles (heuristic subsystem)

| Title ID | C+D rows | Subsystem guess | Sample patch |
|----------|----------|-----------------|--------------|
| 5454082B | 67 | gpu | Disable Depth of Field & Motion Blur |
| 544307D1 | 28 | unknown | Unlock All Menus |
| 545107FC | 26 | unknown | PS3 Button Prompts |
| 4D5308BC | 23 | unknown | Disable Achievements |
| 4D530A26 | 20 | gpu | Disable Lens Flares |
| 4541080F | 19 | unknown | -dvd |
| 4D5307D5 | 19 | unknown | Black Shading Fix |
| 4D5307ED | 19 | unknown | Fix crashing on start-up |
| 454108CE | 18 | unknown | Black Shading Fix |
| 4D53082D | 18 | unknown | Black Shading Fix |
| 415607E1 | 17 | unknown | Remove Grass |
| 545107D1 | 17 | unknown | Tree LODs |
| 58410955 | 17 | gpu | HD Shadows |
| 4D5308AB | 16 | gpu | Disable Lens Flares |
| 4D5307E8 | 13 | unknown | Black Shading Fix |
| 545107F8 | 13 | unknown | 1201x675 |
| 545408A7 | 13 | unknown | GTA IV Wanted System |
| 53450812 | 12 | unknown | 1280x720 Resolution |
| 57520802 | 12 | unknown | Debug Menu |
| 57520828 | 12 | unknown | Debug Menu |
| 4D5307FA | 11 | unknown | Disable Depth of Field |
| 454108EF | 10 | gpu | Disable Lens Flares |
| 45410850 | 9 | gpu | Dynamic Lighting & Shadows Shading Fix |
| 545407F8 | 9 | gpu | Disable Motion Blur |
| 565707D0 | 9 | gpu | Disable Lens Flares |

## Likely false-positive category C (~366 rows)

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
