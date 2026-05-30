# Settings UI (Preferences)

Phoenix exposes **xenia-canary.config.toml** (global) and **config/{TitleID}.config.toml** (per-game) through **Settings → Preferences** (`Ctrl+,`).

## Menu map

| Menu item | Preferences tab |
|-----------|-----------------|
| Preferences… (`Ctrl+,`) | Graphics (default) |
| Graphics… (`F7`) | Graphics |
| Video & Display… (`F6`) | Video & Display |
| Audio… | Audio |
| Input… | Input |
| Storage & Paths… | Storage |
| Game overrides… | Game overrides (per-game mode on) |
| Xbox console… | Xbox console |
| Open config file… | Opens config in Explorer |

Profile and XMP remain separate quick dialogs. Library folders stay under **Library**.

## Tabs and TOML sections

| Tab | TOML categories / content |
|-----|---------------------------|
| Graphics | Curated GPU panel + `[GPU]` (auto) |
| Video & Display | Post-process panel + `[Display]`, `[Video]` |
| Audio | `[APU]` |
| Input | `[HID]` |
| Storage | `[Storage]` |
| General | `[General]`, `[UI]`, `[Profiles]` |
| CPU & System | `[CPU]`, `[Kernel]`, `[x64]` |
| Logging | `[Logging]`, `[Debug]` |
| Advanced | `[D3D12]`, `[Vulkan]`, `[Memory]`, `[Win32]`, `[HACKS]`, advanced `[GPU]` |
| Game overrides | Lists keys in the active title’s config file |
| Xbox console | Guest XConfig (not TOML); opens console settings window |

## Per-game overrides

When a title is running, enable **Apply to current game only**. Changes save to `config/<8-digit-title-id>.config.toml` instead of the global file.

Launcher **Configure for this game…** opens Preferences on the Graphics tab with per-game mode enabled.

## Search and advanced options

Each auto-generated tab includes:

- **Search** — filters by cvar name, category, or description
- **Show advanced options** — reveals trace, dump, and compatibility-hack cvars hidden by default

Some options show **(restart)** when a full emulator restart is required (for example GPU backend or internal resolution scale).

## Internal vs output scaling

See [graphics_settings.md](graphics_settings.md) for the distinction between **Graphics → Resolution scale (internal)** and **Video & Display** post-processing (FXAA, CAS, FSR).
