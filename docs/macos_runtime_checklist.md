# macOS runtime checklist (Phase 4.4a)

Manual sign-off on Apple Silicon hardware.

## Launch

- [ ] `xenia_canary --help` exits 0
- [ ] Main window opens (Cocoa + MoltenVK)

## Graphics

- [ ] Vulkan presenter connects to `CAMetalLayer`
- [ ] Boot a legally owned homebrew / smoke title
- [ ] Stable for 5+ minutes

## Audio / input

- [ ] SDL audio audible
- [ ] Controller / keyboard input

## Telemetry

- [ ] Optional stub JSONL on smoke title
- [ ] Record in [60_smoke_titles.md](../../metacache/plan/60_smoke_titles.md) macOS column
