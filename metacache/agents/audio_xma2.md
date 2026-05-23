# Agent: audio_xma2

## Mission

XMA2 fixture parity and empty XMA divergence logs on smoke captures; stable PCM hashes.

## Read first

1. [re/30_xma2_audio.md](../re/30_xma2_audio.md)
2. [plan/60_smoke_titles.md](../plan/60_smoke_titles.md) audio columns
3. [schemas/telemetry.md](../schemas/telemetry.md) APU sections

## Owns

- `tests/xma2_packets/` fixtures
- Smoke XMA / PCM JSONL interpretation

## Commands

```powershell
cd "G:\Xenia-Xenia Canary\Xenia-Phoenix\xenia-phoenix-src"
powershell -File tools/tier0/run_xma2_diff.ps1
python tools/tier0/compare_pcm_baseline.py  # if present in tree
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| Capture needed | `gameplay_smoke` |
| Kernel audio export | `kernel_xam` |

## Stop and ask human

- Bit-exact oracle requires NDA SDK material in-repo
