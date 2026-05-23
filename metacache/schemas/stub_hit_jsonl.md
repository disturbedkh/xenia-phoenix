# Kernel stub-hit JSONL

> **Observability v2:** the same hits are also emitted to unified [obs events](obs_event_v1.json) (`kind=Stub`, `channel=Kernel.Stub`) in `telemetry/{title}_events.jsonl` when `log_preset` is develop/support/homebrew/forensic. Legacy file format below remains for compat.

Subset of [telemetry.md](telemetry.md) §1 — kept for agents that only touch kernel work.

## File

`--kernel_stub_hit_log=<path>` → one JSON object per line.

## Required fields

| Field | Type | Notes |
|-------|------|-------|
| `ts` | string | ISO-8601 |
| `module` | string | e.g. `xboxkrnl.exe` |
| `export` | string | export name |
| `title_id` | string (hex) | current title |
| `lr` | string (hex) | guest link register |

## Optional fields

`ctr`, `r3`, `r4`, `r5`, `detail` — see full schema in `telemetry.md`.

## Aggregation

```powershell
python tools/tier0/aggregate_stub_hits.py telemetry/TTTTTTTT_stubs.jsonl
```

Inventory: `xenia-phoenix-src/docs/kernel_stub_inventory.json`.
