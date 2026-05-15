## GPU trace corpus (Tier 0 / MVP2)

Place **legally captured** `.xtr` files here (e.g. `4D5307D1_title_menu.xtr`).

### How to capture

1. Build canary with `-DXENIA_BUILD_MISC=ON`.
2. Run `xenia-canary.exe` with your test title.
3. Use the in-app GPU trace record hotkey / workflow documented on the
   [Xenia Canary wiki](https://github.com/xenia-canary/xenia-canary/wiki).
4. Copy the resulting `.xtr` into this directory.

### Trace format version

Traces are tied to `kTraceFormatVersion` in
`src/xenia/gpu/trace_protocol.h`. If the emulator bumps the format, **re-capture**
all corpus files.

### CI harness

See [tools/gpu_replay_ci/run.py](../../tools/gpu_replay_ci/run.py).

```powershell
python tools/gpu_replay_ci/run.py --build-dir build --backend d3d12 --cross-path d3d12
```

The `golden/` subdirectory stores the last JSON report from the harness (small;
do not commit large binary goldens without LFS).
