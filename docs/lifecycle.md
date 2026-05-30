# Emulator lifecycle (Phoenix)

Phoenix uses an explicit **lifecycle state machine** so the launcher UI can stay alive while titles start, stop, and relaunch.

## States

| State | Meaning |
|-------|---------|
| `Bare` | Core not initialized |
| `Ready` | Kernel up, no title; launcher visible |
| `Launching` | Mount + load in progress |
| `Running` | Guest main thread active |
| `Paused` | Guest paused |
| `Terminating` | Title teardown |
| `Relaunching` | In-process kernel reset |
| `Respawning` | New OS process (backend change / POSIX) |
| `Exiting` | Application shutdown |

## Entry points

All launches go through `TitleLaunchDispatcher::Dispatch` in
[`src/xenia/app/title_launch_dispatcher.cc`](../src/xenia/app/title_launch_dispatcher.cc).

| Platform | First title | Title already open |
|----------|-------------|-------------------|
| Windows | `SetupSubsystems` + `LaunchPath` | `RelaunchTitle` (lifecycle worker) |
| Windows (gpu/apu change) | — | `LaunchTitleInNewProcess` |
| Linux/macOS | spawn | spawn |
| Android | Activity intent | Activity intent |

## Key APIs

- `Emulator::Setup` — memory, CPU, VFS, kernel, HID shell
- `Emulator::SetupSubsystems` — GPU, APU, media player
- `Emulator::ShutdownSubsystems` — tear down GPU/APU only
- `Emulator::RelaunchTitle` — full in-process relaunch (Win32)
- `Emulator::ResetTitle` — return to launcher idle state
- `Emulator::PostToLifecycleWorker` — joinable worker (not detached)

## UI

- `LauncherDashboardDialog` + `library::GameLibrary` (existing Phoenix UI)
- `on_lifecycle_change` shows/hides the launcher (`Ready` vs `Running`)

## Flags

- `--return_to_ui=true` — set on child processes spawned by `LaunchTitleInNewProcess`
  (backend change or POSIX first launch). Keeps the UI session alive after the title
  exits so the launcher can return (`LifecycleState::Ready`).

## Smoke

```powershell
tools/tier0/run_lifecycle_smoke.ps1 -GameA <path> -GameB <path>
```

## See also

- Metacache: [`metacache/re/50_lifecycle.md`](../../metacache/re/50_lifecycle.md)
