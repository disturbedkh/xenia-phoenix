# RE tools

What the AI uses (or recommends) when reverse-engineering 360 binaries (XEX), Xenia source, and game behavior.

## Static analysis

| Tool | Use | Notes |
|------|-----|-------|
| **IDA Pro** + Hex-Rays PPC plugin | XEX disassembly + decompilation | Best decomp output for PPC. Commercial. |
| **Ghidra** + PPC plugin | XEX disassembly | Free. Decent decompiler. Community PPC scripts available. |
| **Binary Ninja** + PPC plugin | XEX disassembly | Commercial-light. Good API for scripting. |
| **xex2lib** / `xextool` | unpack XEX -> raw PE | Strip XEX2 wrapper, get raw PPC PE. |
| **Xenia source** | implementation oracle | Often the fastest "what does this API do?" source. |

## Dynamic / runtime

| Tool | Use | Notes |
|------|-----|-------|
| **Xenia debug log** + cvars | game runtime tracing | `--log_level=debug`, `--kernel_stub_hit_log=...`. |
| **phoenixctl / phoenix MCP** | agent-driven launch + tail + triage | [dev/55_cursor_live_probe.md](../dev/55_cursor_live_probe.md) |
| **phoenix_probe HTTP** | live status on localhost | `--phoenix_debug_port=8765`, `GET /status` |
| **Xenia GPU traces** (.xtr) | rendering reproducer | Capture in-app via Xenia's debug menu. |
| **PIX on Windows** (D3D12 capture) | host-side GPU debug | Captures Xenia's translated D3D12 calls. Useful for diagnosing translation bugs. |
| **RenderDoc** (D3D12) | host-side GPU debug | Same idea, different UX. |
| **Vulkan validation layers** | catch backend issues | Run with `--gpu=vulkan --vulkan_validation`. |

## Audio

| Tool | Use | Notes |
|------|-----|-------|
| **ffmpeg** (`wmapro` decoder) | XMA2 ~near-oracle | Not bit-exact. Use as a coarse comparison. |
| **Audacity** | inspect Xenia PCM dumps | Spectrogram / waveform diff between runs. |
| **xma2encode/decode** (XDK) | bit-exact XMA2 | Leaked SDK only; do not redistribute. |

## Filesystem / packages

| Tool | Use | Notes |
|------|-----|-------|
| **Velocity** (Xbox360GameSaveCleaner / STFS GUI) | Inspect STFS containers | Open-source-ish. |
| **xeBuild** ecosystem | NAND / TU / DLC | Mostly modded-console oriented; mostly out-of-scope for Phoenix Tier 0/1. |
| **`xb1tool`** (community) | XContent metadata | Read content type, title id, display name. |

## Reference / docs

| Source | What's there |
|--------|--------------|
| **Free60 wiki** (https://free60.org) | Most public RE knowledge for the 360 |
| **xboxdevwiki** | Mostly Xbox 1 but useful for context |
| **Microsoft public docs** | XAudio2, XInput, D3D9 (parent API) — surprisingly close to 360 user-mode |
| **IBM PowerPC manuals** | Book I/II/III; freely downloadable |
| **Xenia Discord** | Live community knowledge (not a code reference, but a debugging help line) |
| **Hidden-Palace** | Legitimate beta / prototype dumps; useful for test corpus |

## Convention for findings in this repo

| Kind | Where | ID |
|------|-------|-----|
| Retail title symptom + repro | [findings/games/](../findings/games/INDEX.md) | `G-<TITLEID>-<nnn>` |
| Cross-title translation pattern | [findings/universal/](../findings/universal/INDEX.md) | `U-<AREA>-<nnn>` |
| Xbox / GPU theory | `re/*.md` | `RE-<nnn>` optional |

Promotion workflow: [findings/INDEX.md](../findings/INDEX.md). GPU native fix bar: [dev/gpu_fix_bar.md](../dev/gpu_fix_bar.md).

## Convention for RE notes in this repo

When the agent learns **architecture or spec** detail, append to the right `re/*.md` file with this structure:

```
## RE-<NNNN>: <short title>

**Observed:** what we saw (concrete: trace ID, address, register state).

**Interpretation:** what we think it means (link to Free60 / source / spec).

**Phoenix impact:** which file in the canary fork is affected; suggested fix or experiment.

**Status:** open / investigating / resolved-in-PR-#NN.
```

This keeps the brain searchable and lets multiple sessions build on each other.
