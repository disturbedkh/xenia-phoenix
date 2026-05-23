# Agent: vfs_content

## Mission

STFS, SVOD, and xcontent package behavior; VFS mount issues blocking title boot.

## Read first

1. [re/50_stfs_xcontent.md](../re/50_stfs_xcontent.md)
2. [re/00_overview.md](../re/00_overview.md)
3. [dev/30_repo_layout.md](../dev/30_repo_layout.md) — `src/xenia/vfs/`

## Owns

- `re/50_stfs_xcontent.md` knowledge updates
- VFS-related fixes in xenia-phoenix-src

## Commands

Standard build per `dev/10_build_commands.md`; reproduce with user-owned content paths only.

## Hand off to

| Blocker | Agent |
|---------|-------|
| Kernel file APIs | `kernel_xam` |
| Boot after mount OK | `gameplay_smoke` |

## Stop and ask human

- DRM / decryption scope expansion
