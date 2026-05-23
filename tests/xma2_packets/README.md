# XMA2 packet fixtures (Phase 1.4)

Legal sources only:

- Synthetic packets from `tools/xma2_diff/gen_fixture_packets.py`
- Your own Xbox 360 captures (not committed here)
- Minimal hand-built headers for structural tests (skip, metadata)

## Layout

Each case is a pair:

- `<id>.xma` — raw 2048-byte packet(s); multi-packet cases concatenate packets.
- `<id>.json` — metadata: `sample_rate_hz`, `channels`, `sample_rate_id`, `is_stereo`, `packet_count`, optional `mode`, optional `xma_frame_hex`.

Regenerate:

```powershell
python tools/xma2_diff/gen_fixture_packets.py
```

## CI

`xma2-diff.exe` reads this directory (default) and exits 0 when there are zero divergences.
