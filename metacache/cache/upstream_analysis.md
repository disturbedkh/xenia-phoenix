# Upstream update analysis for Xenia Phoenix

Generated: 2026-05-23 11:22
Phoenix HEAD: `038999f` (038999f79ccce31fd7ea5e487b48e4ce87170c05)
Window: commits since 2026-05-09 (14 days)
Watch paths: src/xenia/apu, src/xenia/base, src/xenia/cpu, src/xenia/gpu, src/xenia/hid, src/xenia/kernel, src/xenia/debug, tools/phoenixctl, tools/phoenix-mcp, tools/tier0, tools/vmx128_fuzz, tools/gpu_replay_ci, game-patches, docs

## xenia-canary (https://github.com/xenia-canary/xenia-canary.git)

- Upstream `origin/canary_experimental`: `09dbe2cd3`
- Merge-base with Phoenix: unavailable (shallow clone or unrelated history)

| Commit | Date | Author | Subject |
|--------|------|--------|---------|
| `09dbe2cd3` | 2026-05-21 | oreyg | [APU] XmaContextNew: do not invalidate output_buffer_valid prematurely |
| `b2aa8b200` | 2026-05-20 | Adrian | [BASE] Use FlushAllSinks instead of inline loop |
| `a11908fa7` | 2026-05-19 | Gliniak | [XAM] Enable storing IPTV name. |
| `072ef7eff` | 2026-05-19 | Gliniak | [XAM] Implemented XamGetOnlineLanguageAndCountry |
| `9ce91e55f` | 2026-05-19 | Gliniak | [UI] Listed extended languages |
| `99ea6da18` | 2026-05-17 | Gliniak | [XConfig] Implementation of XConfig |
| `f88bfbe41` | 2026-05-18 | The-Little-Wolf | [Xam/Content] - Implement XamContentCreateEnumeratorInternal |
| `ef67d1ca3` | 2026-05-18 | Gliniak | [SMC] Removed std::bind usage in favor of lambda |
| `482629a3b` | 2026-05-17 | Gliniak | [XAM] Fixed issue with different param count in XamFormatDateString and XamFormatTimeString |
| `a261b83de` | 2026-05-14 | Adrian | [APP] Flush log & save config on quick exit |
| `4c396fe61` | 2026-05-16 | Gliniak | [XAM] Implemented XamGetLanguageLocaleFallbackString and XamGetLanguageTypeface |
| `dc4db67f9` | 2026-04-11 | The-Little-Wolf | [Kernel] - Change OddObj to X_DISPATCH_HEADER |
| `c2674b19d` | 2026-05-15 | oreyg | [Vulkan] Drop vertex buffer residency cache, hoist global lock like D3D12 |
| `b15fcc73e` | 2026-05-15 | oreyg | [Vulkan] Route guest oDepth through gl_FragDepth in FBO and into FSI depth test |
| `6181160ed` | 2026-05-14 | Adrian | [XAM] Fixed XEX swap failing to remove prefix |
| `562aa0dd9` | 2026-05-15 | oreyg | [Vulkan] Only declare the clip-distance or cull-distance array that is written |
| `02a259129` | 2026-05-15 | oreyg | [Vulkan] Port ac6_ground_fix vertex-fetch nudge to SPIR-V |
| `cebbdb6ca` | 2026-05-14 | Adrian | [Kernel] Implemented XeCryptHmacShaInit, XeCryptHmacShaUpdate and XeCryptHmacShaFinal |
| `fdbaaaba2` | 2026-03-27 | Gliniak | [Kernel] Replace SHIM version of kernel strings with exports |
| `b575c6841` | 2026-05-12 | oreyg | [XMA] Enhances the existing XmaContextNew implementation with one derived from the AC6_recomp RexGlue SDK. Key fixes versus the previous implementation: |
| `d1e587617` | 2026-05-13 | Adrian | [UI] Fallback to default button for XamShowMessageBoxUI |
| `331ddf79b` | 2026-05-13 | Gliniak | [Kernel] KeGetImagePageTableEntry: Added bit 0 set |
| `cdb88d0b3` | 2026-05-13 | Gliniak | [Kernel] Remove IsQueued check for DPC |
| `f3086cfce` | 2026-05-03 | Guilhem Massol | [Compilation] Fix Clang 23 |
| `f4f821215` | 2026-05-09 | Adrian | [XGI] Stubbed XUserAwardAvatarAssets |
| `bfdaa4182` | 2026-05-09 | Adrian | [XGI] Cleanup XUserWriteAchievements |

## Next steps

1. Triage watch-path commits against [edge_port_queue.md](../Xenia-Phoenix/metacache/plan/edge_port_queue.md).
2. Run targeted diffs: `git diff --no-index` or `git log -p` on specific files.
3. Port class **A** fixes; queue **B** after smoke; skip **C** Phoenix deliberate deltas.
