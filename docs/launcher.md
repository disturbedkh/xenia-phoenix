# Phoenix Launcher

Xenia Phoenix opens to the **launcher dashboard** when no game is running.

## Adding games

1. **Library → Add game folder...** (or **Library → Open launcher** then **Add folder...**)
2. Choose a directory containing Xbox 360 titles (`.xex` / `default.xex`, `.iso`, `.zar`, STFS packages)
3. Click **Rescan library**

Entries are stored in `library.toml` under the emulator storage root. Cover art is cached under `cache/icons/<title_id>.png` when available from package metadata.

## Playing

- Double-click a tile or use **Play** in the context menu
- **F9** returns to the launcher while a title is running (emulation is paused)

## Per-game settings

Right-click a tile → **Configure for this game...** to open **Settings → Graphics** with per-title config saved to `config/<title_id>.config.toml`.
