# Agent: kernel_xam

## Mission

Drain kernel/XAM stub hits on smoke titles by implementing shims, not patches.

## Read first

1. [re/40_kernel_xam.md](../re/40_kernel_xam.md)
2. [schemas/stub_hit_jsonl.md](../schemas/stub_hit_jsonl.md)
3. [plan/60_smoke_titles.md](../plan/60_smoke_titles.md)
4. [xenia-phoenix-src/docs/kernel_stub_inventory.json](../../xenia-phoenix-src/docs/kernel_stub_inventory.json)

## Owns

- `src/xenia/kernel/**` changes (in xenia-phoenix-src)
- Stub inventory JSON when exports promoted

## Commands

```powershell
cd "G:\Dev\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
python tools/tier0/aggregate_stub_hits.py telemetry/TTTTTTTT_stubs.jsonl
```

## Hand off to

| Blocker | Agent |
|---------|-------|
| Need 5-min capture | `gameplay_smoke` |
| Patch retirement | `patch_debt` |

## Stop and ask human

- Deleting shim with unknown callers
- IOCTL behavior needs hardware verification
