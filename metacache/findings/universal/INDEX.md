# Universal translation patterns

Cross-title patterns. Each file is a **catalog** of `U-*` entries. Link **from** [games/](../games/INDEX.md) and **to** [re/](../../re/INDEX.md) theory.

| File | Area | Topics |
|------|------|--------|
| [gpu_edram.md](gpu_edram.md) | GPU | EDRAM ownership, host depth 20e4, uploads, resolves, traces |
| [kernel_xam.md](kernel_xam.md) | KRN | Stubs, xboxkrnl/xam behavior (index) |
| [apu_xma.md](apu_xma.md) | APU | XMA/PCM drift (index) |

## Pattern status key

| Status | Meaning |
|--------|---------|
| `open` | Hypothesis or known gap, no src fix |
| `confirmed` | Reproduced on 2+ titles or synthetic corpus |
| `fix-landed` | Merged in `src/`, cvars can retire |
| `wont-fix` | Documented limitation |

## Add a pattern

1. Copy [_template_universal.md](../_template_universal.md) section into the right area file.
2. Assign next `U-<AREA>-<nnn>` ID.
3. Link all affected `G-*` game entries.
4. When fixing: drift log row + replay gate ([gpu_fix_bar.md](../../dev/gpu_fix_bar.md)).
