# Phoenix tools

Tier 0 and observability tooling live under `xenia-phoenix-src/tools/`. **Edit tools only in that tree** — no duplicate copies under `metacache/`.

## tier0/

Scripts and Python utilities for bootstrap, patch categorization, smoke capture, and CI helpers.

| Artifact | Purpose |
|----------|---------|
| `bootstrap.ps1` | One-shot environment setup |
| `categorize_patches.py` | Patch debt categorizer → JSON dashboard |
| `run_smoke_capture.ps1` | ~5 min telemetry capture per title |
| `run_vmx128_full_sweep.ps1` | 1M opcode VMX128 sign-off |
| `run_gpu_replay.ps1` | GPU trace replay wrapper |
| `list_smoke_patches.py` | Patches linked to smoke title IDs |

Location: [../xenia-phoenix-src/tools/tier0/](../xenia-phoenix-src/tools/tier0/)

## phoenixctl

CLI for telemetry launch, tail, triage, and Tier 0 gates.

```powershell
cd xenia-phoenix-src
pip install -e tools/phoenixctl
phoenixctl preflight
```

Docs: [../xenia-phoenix-src/tools/phoenixctl/README.md](../xenia-phoenix-src/tools/phoenixctl/README.md)

## phoenix-mcp

stdio MCP server exposing Phoenix telemetry tools to **Cursor**.

Setup: [../metacache/workspace/03_cursor_mcp_setup.md](../metacache/workspace/03_cursor_mcp_setup.md)

```powershell
cd xenia-phoenix-src/tools/phoenix-mcp
pip install -e ../phoenixctl
pip install -e .
python -m phoenix_mcp.server
```

Live probe workflow: [../metacache/dev/55_cursor_live_probe.md](../metacache/dev/55_cursor_live_probe.md)

## vmx128_fuzz

Differential fuzzer comparing JIT vs reference for VMX/VMX128 opcodes.

Location: [../xenia-phoenix-src/tools/vmx128_fuzz/](../xenia-phoenix-src/tools/vmx128_fuzz/)

## gpu_replay_ci

GPU trace replay for RTV/ROV hash parity.

| Script | Role |
|--------|------|
| `run.py` | Replay + compare |
| `validate_traces.py` | Corpus format validation |
| `gen_phase12_fixture_xtr.py` | Synthetic fixture generator |

Location: [../xenia-phoenix-src/tools/gpu_replay_ci/](../xenia-phoenix-src/tools/gpu_replay_ci/)

## Observability

Instrumented launches use **observability v2** presets. Events follow `obs_event_v1` schema.

- [../metacache/dev/observability.md](../metacache/dev/observability.md)
- [../metacache/schemas/obs_invariants.md](../metacache/schemas/obs_invariants.md)
- Local captures: `xenia-phoenix-src/telemetry/` (gitignored)

## phoenix_probe (HTTP)

In-process debug HTTP on `127.0.0.1` (default disabled). Schema: [../metacache/schemas/phoenix_probe_http.md](../metacache/schemas/phoenix_probe_http.md)

## Deeper detail (agents)

- phoenixctl reference: [../metacache/dev/56_phoenixctl_reference.md](../metacache/dev/56_phoenixctl_reference.md)
- MCP tool schemas: [../metacache/schemas/mcp_tools.md](../metacache/schemas/mcp_tools.md)
