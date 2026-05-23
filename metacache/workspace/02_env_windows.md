# Windows dev environment

## Prerequisites

- Visual Studio 2022 with C++ desktop workload
- CMake on **PATH** (see pitfall P-001 in `dev/60_known_pitfalls.md`)
- Vulkan SDK (e.g. `C:\VulkanSDK\1.4.350.0`)
- Python 3.x on PATH

## Typical build shell

```powershell
cd "G:\Dev\The Xenia Project\Xenia-Phoenix\xenia-phoenix-src"
# Open "x64 Native Tools Command Prompt for VS 2022" OR:
& "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$env:VULKAN_SDK = "C:\VulkanSDK\1.4.350.0"
python xenia-build.py build
```

Or use `tools/tier0/bootstrap.ps1` per `dev/10_build_commands.md`.

## Outputs

- Release binaries: `build/bin/Windows/Release/`
- `vmx128-fuzz.exe`, `xenia-cpu-tests.exe`, `xenia_canary.exe`

## Common failures

| Symptom | See |
|---------|-----|
| `cmake` not found | `dev/60_known_pitfalls.md` P-001 |
| `0xC0000135` on test exe | Missing DLL — run from VS dev shell or copy deps |
| Path with spaces breaks script | Quote paths; see `workspace/01_canonical_paths.md` |
