# Translation findings — per-game and universal

Long-term cache for **guest behavior vs emulator translation** observations. Goal: spot patterns across titles and promote fixes from title workarounds → **`src/` universal behavior**.

## Two layers

| Layer | Path | What belongs here |
|-------|------|-------------------|
| **Per-game** | [games/](games/INDEX.md) | Symptoms, repro, telemetry paths, title-specific cvars/patches, links to traces |
| **Universal** | [universal/](universal/INDEX.md) | Subsystem patterns that explain *classes* of bugs (EDRAM ownership, depth 20e4, stubs) |

**RE theory** stays in [re/](../re/INDEX.md). **Findings** are empirical: what we saw on hardware-in-emu, what fixed it, and whether the fix generalizes.

## ID conventions

| Prefix | Example | Use |
|--------|---------|-----|
| `G-<TITLEID>-<nnn>` | `G-454107DB-001` | One observation tied to a retail title |
| `U-<AREA>-<nnn>` | `U-GPU-003` | Pattern that may apply to many titles (`AREA`: GPU, KRN, APU, VFS, CPU) |

Always link game findings to universal IDs when a pattern matches (`promotes_to: U-GPU-002`).

## Promotion ladder (game → universal)

```text
observe (game) → classify (universal?) → prove (corpus) → fix (src) → retire (cvar/patch)
```

| Stage | Gate | Artifact |
|-------|------|----------|
| **Observe** | Repro + telemetry | Game note `G-*` entry |
| **Classify** | Same mechanism on 2+ titles OR matches RE doc | Universal note `U-*`; link from games |
| **Prove** | RTV/ROV replay or drift log | [re/20_xenos_gpu.md](../re/20_xenos_gpu.md) drift row; `.xtr` in CORPUS (not committed if retail) |
| **Fix** | No title-only `#ifdef` unless detection is generic | PR in `xenia-phoenix-src/src/` |
| **Retire** | Game plays without launch cvars | Update game note `status: resolved`; debt ledger |

Title launch scripts may use **workarounds** for playtesting; they are not **done** until the ladder reaches Fix + Retire.

## Session workflow

1. Play / triage with [phoenixctl](../dev/56_phoenixctl_reference.md) or tier0 launch scripts (`--log_preset=develop` on instrumented paths).
2. Append to the game file under `games/<titleid>.md` using [\_template_game.md](_template_game.md).
3. If the observation is not title-specific, add or extend a row in [universal/](universal/INDEX.md).
4. On milestone: one bullet in [cache/session_log.md](../cache/session_log.md) with finding IDs.
5. Handoff: [meta/01_handoff.md](../meta/01_handoff.md) (findings row added).

## Discovery vs proof loops

| Loop | Tools | Feeds |
|------|-------|--------|
| **Obs summary** | `phoenixctl log summarize --write-summary` | `telemetry/{tid}_obs_summary.json`, BF2 `classification` |
| **Discovery** | `phoenixctl repro capture` (or `tools/tier0/gpu_repro_capture.ps1`), probe `/snapshot`, F4 `.xtr` | Game notes, new `G-*` |
| **Proof** | `run_gpu_replay.ps1`, `validate_traces.py`, RTV/ROV hash | Universal notes, `U-*`, drift log |

Standard handoff: **`phoenixctl repro capture --title-id <ID> --post-only`** after F4 at repro.

See [dev/gpu_fix_bar.md](../dev/gpu_fix_bar.md) for the minimum bar before calling a GPU fix "universal."

## Quick links

- Smoke roster: [plan/60_smoke_titles.md](../plan/60_smoke_titles.md)
- GPU agent: [agents/gpu_edram.md](../agents/gpu_edram.md)
- Compat tracker (public): [links/curated_urls.yaml](../links/curated_urls.yaml) — add game-compat issue URLs per title
