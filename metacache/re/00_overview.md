# Xbox 360 system overview

Quick architectural model the AI uses to map source files to hardware.

## High-level block diagram

```
+---------------------+      +---------------------+      +---------------------+
|       Xenon CPU     |      |     Xenos GPU       |      |   System / I/O      |
|  3 x PPC cores @    |<---->| ATI R500 derivative |<---->| HDD, DVD, USB, NIC, |
|  3.2 GHz, SMT2,     |  FSB |  10 MB EDRAM,       |      | Audio I/O, NAND     |
|  shared 1MB L2      |      |  unified shaders    |      |                     |
|  + VMX128 unit      |      |                     |      |                     |
+---------------------+      +---------------------+      +---------------------+
         ^                            ^                            ^
         |                            |                            |
         +----------------------------+----------------------------+
                              512 MB GDDR3 (unified)
```

## Phoenix maps each block to source areas

| Block | Real hardware | Xenia source area |
|-------|---------------|-------------------|
| Xenon CPU | 3-core PPC + VMX128 | `src/xenia/cpu/` (frontend), `src/xenia/cpu/backend/{x64,a64}/` (host JIT) |
| Xenos GPU | R500-derived, EDRAM | `src/xenia/gpu/`, `src/xenia/gpu/{d3d12,vulkan}/` |
| Audio (XMA2) | XMA2 hardware decoder | `src/xenia/apu/`, `src/xenia/apu/xma_*.cc` |
| Kernel | xboxkrnl + xam + xbdm | `src/xenia/kernel/{xboxkrnl,xam,xbdm}/` |
| FS / packages | STFS, SVOD, content | `src/xenia/vfs/` |
| Memory | 512 MB unified | `src/xenia/memory.cc` |
| Patcher | guest-memory overrides | `src/xenia/patcher/` |
| HID | wired/wireless gamepad | `src/xenia/hid/` |

## Things to remember

- **Big-endian.** Guest is PowerPC big-endian; host is little-endian. The canary code treats this carefully — never assume native byte order.
- **Unified memory.** No CPU/GPU memory split; the GPU sees the same 512MB. EDRAM is **separate**, on-die GPU scratch.
- **VMX128, not VMX/AltiVec.** Microsoft extended AltiVec for the 360. There are 128 vector registers (vs. 32 in stock PPC), an extra dot-product opcode set, and re-encoded forms. **Vector accuracy is the #1 emulator-bug source.**
- **EDRAM is the second-largest source of bugs.** Tile mode + resolves + MSAA + memexport are non-trivial.
- **The kernel is small but ubiquitous.** ~1500 ordinals across xboxkrnl/xam; Xenia stubs many. See `40_kernel_xam.md`.
