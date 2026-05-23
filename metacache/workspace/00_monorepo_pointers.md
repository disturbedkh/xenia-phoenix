# Monorepo pointers (read-only)

Phoenix metacache is **Phoenix-only**. Sibling trees are context, not second brains.

| Path | GitHub | Branch | Role |
|------|--------|--------|------|
| `Xenia-Phoenix/xenia-phoenix-src/` | [disturbedkh/xenia-phoenix](https://github.com/disturbedkh/xenia-phoenix) | `canary_experimental` | **Canonical** Phoenix build + git |
| `Xenia-Phoenix/metacache/` | [disturbedkh/xenia-phoenix](https://github.com/disturbedkh/xenia-phoenix) | `metacache` | Agent memory (this tree) |
| `../../xenia-canary/` | [xenia-canary/xenia-canary](https://github.com/xenia-canary/xenia-canary) | `canary_experimental` | Reference canary snapshot |
| `../../xenia-master/` | [xenia-project/xenia](https://github.com/xenia-project/xenia) | `master` | Archival upstream Xenia |
| `../../Xenia-Edge/` | [has207/xenia-edge](https://github.com/has207/xenia-edge) | `edge` | Edge fork (Windows/Linux focus); port queue — [edge_port_queue.md](../plan/edge_port_queue.md) |
| `../../XeniOS/` | [xenios-jp/XeniOS](https://github.com/xenios-jp/XeniOS) | `xenios` | Apple-focused fork |

## Workspace sync (multi-root)

Machine-readable upstream list: [`workspace/upstreams.yaml`](../../../workspace/upstreams.yaml) (workspace root).

```powershell
# From g:\Xenia-Xenia Canary
.\scripts\sync-upstreams.ps1              # fetch all clones
.\scripts\sync-upstreams.ps1 -Pull          # fetch + fast-forward
.\scripts\analyze-upstream-updates.ps1      # triage commits for Phoenix
```

## Remotes (xenia-phoenix-src)

- **origin** → `https://github.com/disturbedkh/xenia-phoenix.git` (push Phoenix work)
- **upstream** → `https://github.com/xenia-canary/xenia-canary.git` (rebase / merge base)

## External plan (read-only)

`c:\Users\khutt\.cursor\plans\xenia_tier_0_execution_plan_94722ff4.plan.md` — do not edit per user instruction.
