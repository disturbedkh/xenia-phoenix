# References (curated)

Public, citable resources. The agent should prefer these before guessing.

## Architecture

- **Free60 — System Software**: https://free60.org/System-Software/
  - CPU: https://free60.org/System-Software/CPU/
  - GPU: https://free60.org/System-Software/GPU/
  - Modules: https://free60.org/System-Software/Modules/
- **Free60 — Formats**: https://free60.org/System-Software/Formats/
  - STFS: https://free60.org/System-Software/Formats/STFS/

## CPU (PowerPC)

- IBM **PowerPC Architecture, Book I/II/III** (Power ISA 2.02 era is closest to 360). Free PDFs; search "PowerPC Architecture Book I PDF".
- AltiVec / VMX programming: IBM **AltiVec Technology Programming Environments Manual**.

## GPU

- AMD/ATI **R500 family** programming references for desktop variants (partially applicable; Xenos has 360 extensions on top).
- Henry de Valence's writeups on Xenos RE (search "henry de valence xenos").
- Xenia GitHub issues and PRs labeled `gpu` / `edram` / `rov`: high-signal RE narrative.

## Audio

- Microsoft **XMA2 Programming Guide** (XDK; leaked-only — use as RE reference, do not paste).
- WMA Pro patent disclosures on Microsoft OpenSpecs.

## Kernel

- Xbox 360 **XDK API Reference** (leaked-only; RE reference only).
- Free60 module list (above) — covers public ordinal map.

## Tools

- **Xenia repo**: https://github.com/xenia-canary/xenia-canary (canary, the active fork).
- **Xenia issue tracker**: highest-quality public bug log for the emulator.
- **xenon-recomp** / **xenia-recomp** (community projects that statically recompile XEX -> native): adjacent to Phoenix; not a dependency.

## Community

- **Xenia Discord** (canary): real-time help.
- **r/xboxone** / **r/originalxbox** subreddits: low signal, occasional gems.

## Phoenix-specific (this repo)

- `xenia-canary/docs/TIER0_README.md` — Phoenix Tier 0 pipeline doc.
- `xenia-canary/docs/TIER0_PR_TEMPLATE.md` — Phoenix PR template.
- `xenia-canary/docs/patch_debt_dashboard.md` — patch categorization spec.

## Citation rule

When pulling from leaked SDK material or non-redistributable docs, **do not** paste passages into source or notes. Cite "see XDK XMA2 docs" and paraphrase. Phoenix must remain redistributable.
