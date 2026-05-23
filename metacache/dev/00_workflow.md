# Agent workflow

End-to-end recipe for Phoenix agents. Follow this loop.

## Loop

```
[1] Pick a task           -> from plan/20_tier_roadmap.md (current phase)
[2] Read RE notes         -> metacache/re/<subsystem>.md (see re/INDEX.md)
[3] Build                 -> dev/10_build_commands.md
[4] Reproduce             -> dev/20_test_commands.md (fuzz / replay / stub-log)
[5] Form hypothesis       -> attach to plan/50_open_questions.md if non-trivial
[6] Implement             -> xenia-phoenix-src/, smallest possible diff
[7] Verify                -> dev/20_test_commands.md (same commands as [4])
[8] Land                  -> dev/40_pr_runbook.md (template, CI, label)
[9] Capture knowledge     -> append to re/*.md and cache/session_log.md
[10] Update debt ledger   -> plan/30_debt_ledger.md if a patch was retired
```

## Where things live

| Kind | Location |
|------|----------|
| Source code | `Xenia-Phoenix/xenia-phoenix-src/` |
| Agent memory | `Xenia-Phoenix/metacache/` |
| Build artifacts | `xenia-phoenix-src/build/` |
| Telemetry (JSONL) | `xenia-phoenix-src/telemetry/` (gitignored) |
| GPU traces | `xenia-phoenix-src/tests/gpu_traces/` |

See [workspace/01_canonical_paths.md](../workspace/01_canonical_paths.md).

## Mental model

- **Phoenix == Xenia Canary + accuracy-improving deviations tracked in metacache.**
- C++ work and tools live in `xenia-phoenix-src/`, not under `metacache/`.

## Anti-patterns

1. Implementing a patch instead of fixing the emulator. (See `plan/30_debt_ledger.md`.)
2. Adding a `// HACK` without a roadmap or open-question entry.
3. Disabling a CI step to ship.
4. Pasting from leaked SDK / NDA docs. Paraphrase + cite, never copy.
5. Editing `c:\Users\khutt\.cursor\plans\xenia_tier_0_execution_plan_94722ff4.plan.md` (read-only).
