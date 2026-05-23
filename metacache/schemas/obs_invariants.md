# Observability invariant registry

Generated from `src/xenia/base/obs/obs_invariants.def`. Each code maps to a
`obs::Invariant` / `OBS_INVARIANT` emission on the host.

| Code | Channel | Notes |
|------|---------|-------|
| DepthHostSidecarStale | Gpu.Edram | BF2 depth sidecar stale |
| HostDepthTransferMismatch | Gpu.Edram | Depth transfer without host sidecar |
| HostDepthStore | Gpu.Edram | Host depth stored to EDRAM |
| GpuUploadRangeError | Gpu.Edram | Invalid GPU upload range |
| PM4UnimplementedOpcode | Gpu.Pipeline | Unimplemented PM4 opcode |
| Msaa2xFallback | Gpu.Pipeline | 2x MSAA emulated via 4x |
| RtFormatUnknown | Gpu.Edram | Unknown render target format |
| EdramBufferAllocFail | Gpu.Edram | EDRAM buffer creation failed |
| ResolveScaledFallback | Gpu.Pipeline | Unscaled resolve fallback |
| MemexportFormatUnsupported | Gpu.Pipeline | Unsupported memexport format |
| TextureFormatUnsupported | Gpu.Edram | Unsupported texture format in frame |
| XmaDivergence | Apu.Xma | XMA divergence site |
| XmaCodecNotFound | Apu.Xma | XMA codec missing |
| XmaAllocContextFail | Apu.Xma | XMA context alloc failed |
| XmaAllocFrameFail | Apu.Xma | XMA frame alloc failed |
| XmaInvalidPacketIndex | Apu.Xma | XMA invalid packet index |
| XmaNoBitsToCopy | Apu.Xma | XMA empty bitstream |
| PipelineNotReady | Gpu.Pipeline | Graphics pipeline not ready |
| VfsResolveFail | Kernel.Stub | VFS path resolve failed |
| KernelStubHit | Kernel.Stub | Kernel/XAM stub hit |

`phoenixctl log summarize` uses `INVARIANT_TO_FINDING` in `log_obs.py` for BF2
classification; extend that table when adding game-specific findings.
