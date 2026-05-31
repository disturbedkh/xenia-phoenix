# Unified Xbox 360 Experience (Phoenix)

Phoenix integrates patterns from [Xenia Manager](https://github.com/xenia-manager/xenia-manager) and [Xenia Dashboard](https://github.com/ALHROOBIX/Xenia-Dashboard) into the native C++ launcher.

## Boot flow

1. **Splash / update check** — existing `Updater` on launch
2. **First-run wizard** — profile, game folder, update channel (`first_run_wizard`)
3. **Dashboard** — tabbed blades: Home, Games, Social, Settings
4. **Launch** — lifecycle `TitleLaunchDispatcher`
5. **Guide overlay** — Tab or LB+Start during gameplay (`guide_overlay`)

## Modules

| Module | Purpose |
|--------|---------|
| `launcher_dashboard` | Blade UI, grid, search, gamepad nav |
| `game_detail_panel` | Metadata, patches, content, launch |
| `game_metadata` | x360db cache |
| `cover_art_fetcher` | Cover cache (+ SteamGridDB when keyed) |
| `patch_catalog` / `patch_manager_panel` | Per-title patch toggle |
| `content_manager` | DLC/TU scan + install |
| `social_panel` | Friends, netplay entry points |
| `guide_overlay` | In-game Xbox Guide |
| `first_run_wizard` | Initial setup |
| `theme_manager` | NXE / Blades / Phoenix presets |
| `imgui_gamepad_nav` | Controller navigation |
| `profile_backup` | XUID-scoped save export |

## Reference repos (read-only)

Cloned at workspace root:

- `xenia-manager/` — C#/Avalonia management patterns
- `xenia-dashboard/` — console UX / Guide / themes

See metacache: `workspace/04_xenia_manager_reference.md`, `workspace/05_xenia_dashboard_reference.md`.

## Hotkeys

| Input | Action |
|-------|--------|
| F9 | Toggle dashboard |
| Tab (in-game) | Guide overlay |
| LB / RB | Switch dashboard blade (gamepad) |
| A | Launch / confirm |
| B | Back |
| X | Configure game |
| Y | Manage |
