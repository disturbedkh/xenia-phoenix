# Xenos GPU + EDRAM

## Core facts

- **ATI Xenos** (codename "C1"): R500 derivative, **first** unified-shader-architecture GPU shipped in a console (predates PC Radeon HD 2000 series).
- **48 unified ALUs**, 16 ROPs, 8 vertex fetch, 16 texture units.
- **10 MB EDRAM** with peak 256 GB/s effective fill rate (write to EDRAM is "free" because it is local).
- **Tile mode + predicated tiling**: scenes wider than EDRAM are rendered tile-by-tile with predication.

## EDRAM in two sentences

EDRAM is small, fast, on-die memory used as the **render target** during draws. After draws, you **resolve** EDRAM into main GDDR3 memory in a tiled or linear texture format. This resolve step has many subtleties (MSAA collapse, format conversion, gamma).

## EDRAM tile geometry (Xenia constants)

From `src/xenia/gpu/xenos.h` (`kEdramTileWidthSamples`, `kEdramTileHeightSamples`, `kEdramTileCount`):

- **Tile size:** 80×16 samples per tile (as counted in the emulator’s EDRAM address space).
- **Tile count:** 2048 tiles span the **10 MiB** EDRAM (`kEdramSizeBytes` = tile count × tile height × tile width × 4 bytes per sample).
- **Trace snapshots:** `EdramSnapshotCommand` in `trace_protocol.h` stores EDRAM as a **sequence of tiles** with **row-major** samples; **2× MSAA** is laid out as **1×2** samples per pixel, **4×** as **2×2** (see struct comment there).

Finer **pitch / base-tile** bitfields (`kEdramPitchPixelsBits`, `kEdramBaseTilesBits`, etc.) live beside those constants for resolve and tiling math in `d3d12_render_target_cache.cc`.

## Why this is the GPU bug epicenter

| Concept | Why hard to emulate |
|---------|---------------------|
| EDRAM tiling | Custom tile format Microsoft never publicly specced. Reverse-engineered. |
| MSAA resolve | 2x/4x MSAA collapsed during resolve, with format conversion. |
| Predicated tiling | Same draw calls run multiple times with viewport offsets; emulator must replicate. |
| memexport | Shader can write structured data to general memory mid-draw. Modern GPUs can fake this with UAVs/SSBOs but with semantic mismatches. |
| Hi-Z / Hi-Stencil | Xenos has its own variants; not directly mappable to D3D12/Vulkan. |
| Color expand-bias | Xenos has hardware bias semantics on some formats that DX12/VK do not. |
| Float-depth math | 360 used a non-standard depth encoding for some titles (24/8 vs. 32f). |

## Xenia's two GPU paths

`src/xenia/gpu/d3d12/d3d12_render_target_cache.cc` exposes two paths via cvar `render_target_path_d3d12`:

- **RTV** (render target views): straightforward D3D12 RTVs. Fast. Inaccurate on resolves, MSAA, custom formats.
- **ROV** (rasterizer-ordered views): emulates EDRAM in a UAV with order-preserving access. Accurate but slow and historically buggier.

Phoenix runs **both** paths from `tools/gpu_replay_ci/run.py` with `--cross-path d3d12`, SHA256-hashes each dump tree, and **exits non-zero** on mismatch unless `--allow-rtv-rov-drift` is set.

## .xtr trace format

`kTraceFormatVersion` is **1** (`src/xenia/gpu/trace_protocol.h`). Bump when the on-disk layout or recorded CP semantics change.

`src/xenia/gpu/trace_protocol.h` defines:

- `kPrimaryBufferStart` / `kPrimaryBufferEnd`
- `kIndirectBufferStart` / `kIndirectBufferEnd`
- `kPacketStart` / `kPacketEnd`
- `kMemoryRead` / `kMemoryWrite`
- `kEdramSnapshot`
- `kRegisters`
- `kEvent` (swap, etc.)
- `kGammaRamp`

Each command has a header followed by typed payload. The trace player rehydrates GPU state and feeds it to the configured backend (`d3d12` or `vulkan`).

**Corpus:** see `tests/gpu_traces/CORPUS.md` and `tools/gpu_replay_ci/gen_phase12_fixture_xtr.py` for Phoenix-authored **synthetic** fixtures (swap-only bootstrap) plus the intended matrix for **legal** retail captures.

## Resolve-path debt markers (Tier 1.2 triage)

The fork no longer uses `// HACK` in `d3d12_render_target_cache.cc`; remaining work is tracked as **`TODO(Triang3l)`** (and similar) comments. Inventory (Phoenix tree, line numbers approximate):

| File | Count | Notes |
|------|------:|------|
| `src/xenia/gpu/d3d12/d3d12_render_target_cache.cc` | 8 | ROV default policy, draw color/depth, host RT → shared resolve, NaN propagation, shader mov, debug names, scissor region. |
| `src/xenia/gpu/dxbc_shader_translator.cc` | 5 | Depth-only pixel shader handling, line / non-adaptive quad patches, copies. |

**Tier 1.2 tracking:** zero RTV/ROV hash drift on the **curated** corpus (enforced by `run.py`); map each **resolve-adjacent** `TODO(Triang3l)` in `d3d12_render_target_cache.cc` to a trace-backed row in **Drift log** as fixes land. **Stretch goal:** reduce unresolved resolve-path TODOs to **≤ 3** (Phoenix bootstrap left **8** in that file — see table).

### Drift log (fill as you fix)

Link closed rows to [findings/universal/gpu_edram.md](../findings/universal/gpu_edram.md) `U-GPU-*` when promoted.

| # | Symptom / area | Trace used | Fix summary |
|---|----------------|--------------|-------------|
| — | *No retail-specific drifts closed in this bootstrap pass.* | Synthetic `phoenix_slot_*` | Harness + fixtures only. |
| 1 | BF2 ground flicker | `454107DB_6004.xtr` (symlink corpus) | U-GPU-001 v2+v3: host-depth retain + stale tile resync + depth self-transfer; **mismatch 0, visual still fails** (2026-05-17). Community fix on **Edge** (float24 PS + allow_invalid upload). Next: [bf2_gpu_roadmap.md](../plan/bf2_gpu_roadmap.md). Headless trace-dump AV `0xC0000005` — not a fix gate ([last_report.json](../../xenia-phoenix-src/tests/gpu_traces/golden/last_report.json)). |

## References

- Free60 wiki: https://free60.org/System-Software/GPU/
- ATI / AMD R500 family programming guides (public documentation for desktop R500 is partially applicable; Xenos has 360-specific extensions).
- Henry de Valence's reverse-engineering posts (search "xenos GPU reverse engineering").
- Xenia issue tracker on GitHub: search for "EDRAM", "ROV", "MSAA", "resolve".

## Phoenix RE backlog for GPU

1. ~~Document the EDRAM tile layout in this file~~ — **Done** (constants + snapshot layout above); extend with bit diagrams per resolve format as fixes land.
2. Document each format conversion in resolves with bit-level diagrams.
3. Document predicated tiling with example trace excerpts.
4. Map each remaining `TODO(Triang3l)` in the render-target path to a numbered row in **Drift log** above once a trace reproduces it.
