# STFS / SVOD / XContent packages

## Core facts

- **STFS** (Secure Transacted File System): Microsoft's container format for downloadable content, profiles, save games, title updates (TUs), arcade titles, and demos.
- **SVOD** (Secure Virtual Optical Disk): container format that simulates a DVD on disk (used for "Games on Demand" disc images).
- **XContent**: the high-level concept of "a package the kernel can mount as a virtual drive."

Each STFS package consists of:

- A 0x971A magic at offset 0 (LIVE / PIRS / CON).
- A header with title metadata, content type, content ID, and one or more **content signatures**:
  - **LIVE**: signed by Microsoft (real content, not redistributable).
  - **PIRS**: signed by Microsoft (system content like avatar assets).
  - **CON**: console-signed (player saves, profiles signed by a specific console).
- Hash tables tracked in 4 KB blocks.
- File listing layout uses a directory tree with allocation chains.

## Xenia handling

`src/xenia/vfs/`:

- `vfs/devices/stfs_container_device.cc` — STFS container reader.
- `vfs/devices/disc_image_device.cc` — disc / SVOD reader.
- `vfs/devices/host_path_device.cc` — direct host filesystem (developer mode).
- `vfs/virtual_file_system.cc` — guest-visible VFS aggregator.

The kernel layer sits on top: `xboxkrnl_io.cc` translates guest `Nt*` calls into `vfs::VirtualFileSystem` operations.

## What's tricky

- **Block chain walks**: STFS allocates files as block chains; emulating sparse / fragmented files needs careful handling.
- **Hash table walks**: STFS validates blocks via hash tables every N blocks.
- **Content type matters**: profile (0x10000) vs. saved game (0x1) vs. title update (0x2000) gate which API surface treats them as which volume.
- **Title-update layering**: TU files override base game files; the kernel's filesystem mounter handles the layering.

## Bug clusters

- DLC mounting failing because the package's content type is not in Xenia's whitelist for that title.
- Save corruption when a write spans block-chain boundaries.
- Achievements / profile mismatches due to STFS signature checks (Xenia mostly skips signature verification).

## Phoenix RE backlog

1. Diagram the STFS header and block chain in this file.
2. Verify that all content types Xenia mounts are in `vfs::devices::content_type_t` enum (or equivalent) and document each.
3. Document the TU-layering precedence rules.

## References

- "Free60 STFS" wiki page: https://free60.org/System-Software/Formats/STFS/
- `Velocity` (Xbox 360 STFS GUI tool): open-source-ish tool that reads STFS; useful as a Python/CLI sanity check.
- `xontools` / `xeBuild` ecosystem (modded community tools).
- Xenia itself: `src/xenia/vfs/devices/stfs_container_device.cc` is a primary reference.

## Legality note

Phoenix tooling will read STFS containers the user already owns (saves, DLC the user purchased). It will **not** include keys, signing material, or anything that enables piracy of Microsoft-signed content the user does not own.
