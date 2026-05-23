# Master (xenia-project/xenia) review

**Repo:** [xenia-master](../../xenia-master/) — branch `master`

**Status (2026-05-23):** Effectively dormant. Last commit **2026-02-18**.

## Triage table

| Commit | Date | Subject | vs Phoenix | Decision |
|--------|------|---------|------------|----------|
| `95a5c3ee` | 2026-02-18 | `[GPU/WGF] Bind EDRAM to resolve dumping via ByteAddressBuffer` | Phoenix Phase 1.2 closed synthetic RTV/ROV corpus on D3D12/Vulkan; WGF debug path is upstream-master specific | **No action** — reference only if retail trace dumping needs WGF parity |

## Notes

- Master is **not** a merge source for Phoenix; use **canary** for living fixes and **Edge** for community harvest ([edge_port_queue.md](edge_port_queue.md)).
- Revisit this row if Phoenix adds WGF backend work or retail `.xtr` capture requires ByteAddressBuffer resolve binding.

## Next review trigger

- Any new commit on `xenia-project/xenia` `master` after 2026-02-18
- Phoenix opens WGF / PIX-style resolve dump tooling
