# Windows ARM64 runtime checklist (Phase 4.3)

Manual sign-off on WoA hardware or ARM64 VM. Automated compile gates: [arm64_status.md](arm64_status.md).

## 4.3.1 — Launch

- [ ] `xenia_canary.exe --help` exits 0
- [ ] Main window opens (ImGui + Vulkan/D3D12 path)
- [ ] No immediate crash on idle

## 4.3.2 — Graphics

- [ ] D3D12 backend initializes
- [ ] Boot a legally owned homebrew / smoke title
- [ ] Stable framebuffer for 5+ minutes

## 4.3.3 — Audio

- [ ] Audio output audible (scalar APU path)
- [ ] No persistent XMA decode spam in log

## 4.3.4 — Input

- [ ] Xbox controller (XInput) recognized
- [ ] Keyboard/mouse if applicable

## 4.3.5 — Telemetry

- [ ] Optional: `--kernel_stub_hit_log=telemetry/<title>_woa.jsonl` for smoke title
- [ ] Record result in [60_smoke_titles.md](../../metacache/plan/60_smoke_titles.md) **Windows ARM64** column

When all rows checked, update [arm64_status.md](arm64_status.md) manual gates to done.
