# Tier 0 pull request template

Use this checklist for compatibility / correctness PRs from the Tier 0 pipeline.

## Summary

- What user-visible behavior changes?

## Root cause

- Which subsystem (`cpu/`, `gpu/`, `kernel/`, `apu/`, …)?
- Why was the old behavior wrong?

## Reproducer

- [ ] `xenia-cpu-tests` addition / `guest_ppc_block_test` / `vmx128-fuzz` run
- [ ] GPU: `.xtr` file + `tools/gpu_replay_ci/run.py` output attached
- [ ] Kernel: `--kernel_stub_hit_log=` JSONL excerpt (if relevant)

## Performance

- Any impact on hot paths? (profiler / trace)

## Risk

- Could this regress other titles? How did you validate?

## Legal / assets

- [ ] No proprietary assets committed (only legally captured traces / homebrew).
