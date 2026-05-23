# RE index — Xbox 360 knowledge

Read [00_overview.md](00_overview.md) first for the system map.

| File | Topic | When to read |
|------|--------|----------------|
| [00_overview.md](00_overview.md) | System architecture | Any new subsystem task |
| [10_xenon_cpu.md](10_xenon_cpu.md) | PowerPC, SMT, general CPU | CPU front-end, exceptions, MMU |
| [10_vmx128.md](10_vmx128.md) | VMX128 fuzz playbook | VMX128 divergences, opcode refs |
| [20_xenos_gpu.md](20_xenos_gpu.md) | EDRAM, resolves, traces | GPU, `.xtr`, RTV/ROV |
| [30_xma2_audio.md](30_xma2_audio.md) | XMA2 packets, APU | Audio decode, smoke XMA logs |
| [40_kernel_xam.md](40_kernel_xam.md) | xboxkrnl, xam, stubs | Kernel shim work |
| [50_stfs_xcontent.md](50_stfs_xcontent.md) | STFS / packages | Mount, content paths |
| [60_re_tools.md](60_re_tools.md) | IDA, Ghidra, BN | RE workflow |
| [70_references.md](70_references.md) | Public URLs | Citations |

## CPU split

- **10_xenon_cpu.md** — general PPC / Xenon behavior.
- **10_vmx128.md** — differential fuzz, pins, FMA, saturation fixes.

## Capture template

- **Theory / architecture:** append to the relevant `re/*.md` file ([60_re_tools.md](60_re_tools.md) structure).
- **Empirical game or cross-title translation:** append to [findings/](../findings/INDEX.md) (`G-*` / `U-*` IDs) and link back here when the mechanism is understood.
