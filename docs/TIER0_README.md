# Tier 0 — PC-only compatibility pipeline

See [TIER0_BUILD.md](TIER0_BUILD.md) for bootstrap, `tools/tier0/bootstrap.ps1`, and
binaries (`vmx128-fuzz`, GPU trace dump tools).

- **MVP1:** `src/xenia/cpu/testing/guest_ppc_test_util.*`, `guest_ppc_block_test.cc`, `tools/vmx128_fuzz/`
- **MVP2:** `tools/gpu_replay_ci/run.py`, `tests/gpu_traces/README.md`
- **MVP3:** `docs/patch_debt_dashboard.md`, `tools/tier0/categorize_patches.py`
- **MVP4:** `--kernel_stub_hit_log=` (`kernel_flags`), `kernel/util/stub_trace.*`
- **PRs:** [TIER0_PR_TEMPLATE.md](TIER0_PR_TEMPLATE.md) — use a long-lived fork branch such as `wip-debt-demolition` for stacked Tier 0 work before opening upstream PRs.
