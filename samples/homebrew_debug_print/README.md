# homebrew_debug_print sample

Minimal documentation for guest `DebugPrint` via PPC trap 20/26.

From guest code (conceptual):

```cpp
void GuestPrint(const char* msg, uint32_t len) {
  // r3 = msg, r4 = len — then trap 20
}
```

Launch host with `--log_preset=homebrew` and read `telemetry/<title>_guest.log`.
