# Homebrew debug output

Use the **homebrew** observability preset so guest strings appear without Phoenix CLI:

```text
xenia_canary.exe title.xex --log_preset=homebrew
```

Output:

- `telemetry/{title}_guest.log` — plain text lines
- `telemetry/{title}_events.jsonl` — structured `Guest.Print` events

## Trap ABI

See `src/xenia/debug/xenia_guest_debug.h`. On Xbox 360 builds, emit trap **20** or **26** with:

- `r3` = string pointer (guest virtual)
- `r4` = length in bytes

The CPU backend implements this in `TrapDebugPrint` (`x64_emitter.cc`).

## Sample

`samples/homebrew_debug_print/README.md` describes a minimal title-side stub.
