# Test commands

## vmx128-fuzz (pin seed + log runs)

After every run, append results to [cache/vmx128_runs.md](../cache/vmx128_runs.md).

```powershell
cd xenia-phoenix-src
$seed = 3735928559
$fuzz = .\build\bin\Windows\Release\vmx128-fuzz.exe

# 5k single opcode (exact name)
& $fuzz --vmx128_fuzz_iters=5000 --vmx128_fuzz_seed=$seed --vmx128_fuzz_filter=^vrsqrtefp

# 50k full registry
& $fuzz --vmx128_fuzz_iters=50000 --vmx128_fuzz_seed=$seed `
  --vmx128_fuzz_report_out=docs/vmx128_fuzz_report_local.json

# 1M sign-off (both passes)
powershell -File tools/tier0/run_vmx128_full_sweep.ps1
```

## CI (Tier 0)

| Workflow | When |
|----------|------|
| `tier0-differential.yml` | Every PR (via Orchestrator) — cpu-tests, vmx128 50k, xma2-diff, GPU replay |
| `tier0-windows.yml` | Manual — full parallel build + same gates |
| `vmx128-1m-weekly.yml` | Weekly / manual — `run_vmx128_full_sweep.ps1` |
| `tier0-checked.yml` | Manual — Checked `xenia-cpu-tests` |

```powershell
# Smoke patch triage (gameplay)
python tools/tier0/list_smoke_patches.py --title-id 4D5307D1 5454082B
```

Guest block tests:

```powershell
.\build\bin\Windows\Release\xenia-cpu-tests.exe "GUEST_PPC_vmx128_vnmsubfp_tuple"
.\build\bin\Windows\Release\xenia-cpu-tests.exe "GUEST_PPC_vrsqrtefp_instr1"
.\build\bin\Windows\Release\xenia-cpu-tests.exe "[guest_ppc][pin]"
```

Checked config (optional):

```powershell
cmake --build build --config Checked --target xenia-cpu-tests -j 8
.\build\bin\Windows\Checked\xenia-cpu-tests.exe
```


The four runtime checks Phoenix uses. Each maps to one Tier 1 phase.

## 1. CPU unit tests (Catch2)

**x64 (default):**

```powershell
cd "G:\Xenia-Xenia Canary\Xenia-Phoenix\xenia-phoenix-src"
.\build\bin\Windows\Release\xenia-cpu-tests.exe
```

**ARM64 (Phase 4 — requires ARM64 build; runs on WoA or x64 via ARM64 emulation):**

```powershell
.\build-arm64\bin\Windows\Release\xenia-cpu-tests.exe
# or after build:
.\tools\tier0\arm64-verify.ps1
```

vmx128-fuzz on ARM64: build with `--target-arch arm64` and `XENIA_BUILD_MISC=ON`; full 1M sweep needs WoA hardware (see [arm64_compat_runbook.md](arm64_compat_runbook.md)).

**macOS (arm64 native):**

```bash
./build/bin/macOS/Release/xenia-cpu-tests
./tools/tier0/macos-verify.sh
```

**Legacy canary path:**

```powershell
cd "G:\Xenia-Xenia Canary\xenia-canary-canary_experimental"
.\build\src\xenia\cpu\testing\Release\xenia-cpu-tests.exe
```

What it covers (after Tier 0 landed): existing HIR vector tests + `guest_ppc_block_test.cc` running guest PPC `vaddubm` against scalar reference.

Add a tag filter:

```powershell
.\build\src\xenia\cpu\testing\Release\xenia-cpu-tests.exe "[guest_ppc]"
```

## 2. VMX128 differential fuzzer

```powershell
.\build\tools\vmx128_fuzz\Release\vmx128-fuzz.exe --iterations=100000 --seed=42
```

Produces stdout summary; nonzero exit on divergence. CI uses a smoke setting (`--iterations=10000`).

Tier 1.1 backlog: extend opcode coverage in `tools/vmx128_fuzz/main.cc`.

## 3. GPU trace replay CI

```powershell
cd "G:\Xenia-Xenia Canary\xenia-canary-canary_experimental"

python .\tools\gpu_replay_ci\run.py `
  --xenia-build .\build\src\xenia\gpu\d3d12\Release `
  --traces .\tests\gpu_traces `
  --report-dir .\build\replay_report `
  --diff-rtv-rov
```

Outputs SHA256 hashes per trace per backend. `--diff-rtv-rov` triggers cross-path RTV vs ROV comparison.

To install the pre-push git hook (local enforcement):

```powershell
.\tools\gpu_replay_ci\install-hooks.ps1
```

## 4. Kernel stub-trace JSONL

Run a title with stub logging:

```powershell
.\build\src\xenia\app\Release\xenia_canary.exe `
  --kernel_stub_hit_log=phoenix_stubs.jsonl `
  "<path-to-XEX-or-iso>"
```

Aggregate after a session:

```powershell
python -c "import json,collections,sys; c=collections.Counter(); [c.update([(j['module'], j['export'])]) for j in (json.loads(l) for l in open('phoenix_stubs.jsonl'))]; [print(f'{n:>6}  {m}!{e}') for (m,e),n in c.most_common(50)]"
```

## 5. Patch-debt dashboard

```powershell
python .\tools\tier0\categorize_patches.py `
  --patches-dir .\game-patches `
  --output .\build\patch_debt.json
```

Then summarize the JSON (top-level fields: total / by_category / per_title).

## 6. Combined "Tier 0 health" check

A loose convention: "everything green" means

- `xenia-cpu-tests.exe` exits 0.
- `vmx128-fuzz.exe --iterations=10000` exits 0.
- `gpu_replay_ci/run.py` reports zero hash drift across traces (or no traces present).
- `patch_debt.json` shows no new category-D entries since last commit.

CI workflow: `.github/workflows/tier0-windows.yml`.
