---
title_id: TTTTTTTT
name: ""
last_verified: YYYY-MM-DD
status: investigating   # investigating | workaround | resolved-partial | resolved-native
compat_issue: ""         # optional GitHub game-compatibility URL
---

# <Name> (`<TITLEID>`)

## Summary

One paragraph: playable? main blockers? best known launch.

## Launch (instrumented)

```powershell
phoenixctl launch triage --title-id <TITLEID> --game "<path>" --fresh --config Debug
$env:PHOENIX_DEBUG_PORT = "8765"
```

Telemetry: `telemetry/<tid>_crash.log`, `_stubs.jsonl`, `_pcm.jsonl`, `_gpu_trace/`.

## Findings log

### G-<TITLEID>-001: <short title>

| Field | Value |
|-------|--------|
| **Observed** | Symptom, scene, build/config |
| **Evidence** | Log lines, probe snapshot fields, trace filename |
| **Interpretation** | Guest vs translation hypothesis |
| **promotes_to** | `U-*` or `—` |
| **Workaround** | Cvars / patches (temporary) |
| **Native fix** | File(s) in `src/` or `open` |
| **Status** | open / workaround / resolved |

## Workarounds in use

| Cvar / patch | Why | Retire when |
|--------------|-----|-------------|
| | | |

## Traces and gates

| Artifact | Path / command |
|----------|----------------|
| In-game `.xtr` | |
| `trace validate` | |
| `run_gpu_replay.ps1` | |
| Release gate | |

## Related

- Universal: 
- RE: 
- Debt: [plan/30_debt_ledger.md](../plan/30_debt_ledger.md)
