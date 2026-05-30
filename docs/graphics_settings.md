# Graphics settings (internal vs output)

For the full settings UI map, see [settings_ui.md](settings_ui.md).

## Internal resolution scale (true 4K)

**Settings → Preferences → Graphics → Resolution scale (internal)** (or **Settings → Graphics**, `F7`)

This multiplies the resolution at which the **GPU renders** the game (`draw_resolution_scale_x/y`). For a typical 720p title, **3×** produces ~**3840×2160** internal pixels (true supersampling).

Requires D3D12 tiled resources or Vulkan sparse binding. Changes apply after **restarting the emulator** or reloading the GPU backend.

This is **not** the same as FSR/CAS under **Settings → Display / Output**, which only filters the finished frame to your window size.

## Display / Output

**Settings → Preferences → Video & Display** (or **Settings → Video & Display**, `F6`)

Post-process **FXAA**, **CAS**, and **FSR 1.0** run after the game has finished rendering. Use **Bilinear** when internal scale already meets your display resolution.

## phoenixctl

```text
phoenixctl library list
phoenixctl library scan
```

Per-game overrides live in `config/<8-digit-title-id>.config.toml`.
