# -Werror parity across platforms

Phoenix treats warnings as errors on every first-party target so a warning on
any OS/arch fails the build (and therefore the CI cell). This documents how that
is enforced and what is intentionally suppressed, so "green on my host" tracks
"green on GitHub."

## Mechanism

`xe_target_defaults(<target>)` in [cmake/XeniaHelpers.cmake](../cmake/XeniaHelpers.cmake)
applies fatal warnings per compiler:

- **MSVC** (Windows x64 + ARM64): `/WX`
- **Clang** (Linux, macOS, Android, and clang-on-Windows): `-Werror`
- **GCC** (Linux alt-toolchain): **no** `-Werror` by design — GCC is too noisy and
  the Linux GCC path additionally sets `-fpermissive`. Clang is the supported
  Linux toolchain (CI uses clang-20); GCC is best-effort only.

It is applied to every `src/xenia/**` module (~35 `CMakeLists.txt` call sites:
`xenia-core`, `xenia-base`, `xenia-cpu`, `xenia-gpu*`, `xenia-kernel`,
`xenia-apu*`, `xenia-hid*`, `xenia-ui*`, `xenia-vfs`, `xenia-app`, backends, etc.).
Third-party code is **excluded** (added under `CMAKE_FOLDER "third_party"`, with
its headers included as `SYSTEM`), so upstream warnings never gate our build.

## Parity matrix

| Cell | Compiler | Fatal warnings |
|------|----------|----------------|
| Windows x64 | MSVC | `/WX` |
| Windows ARM64 | MSVC | `/WX` |
| Linux x64 | Clang 20 | `-Werror` |
| Linux ARM64 | Clang 20 | `-Werror` |
| macOS ARM64/x64 | AppleClang | `-Werror` |
| Android arm64 | NDK Clang | `-Werror` |
| Linux (GCC) | GCC | none (by design) |

No first-party cell silently drops fatal warnings except the unsupported GCC path.

## Intentional suppressions

Global `-Wno-*` / `/wd*` live in the root [CMakeLists.txt](../CMakeLists.txt). They
are the *same set* across the matching compiler, so a suppression on one Clang
cell applies to all Clang cells (parity by construction):

- **MSVC**: `/wd4201` (nameless struct/union); `/utf-8`.
- **All non-MSVC (C++)**: `-Wno-switch`, `-Wno-attributes`.
- **Clang**: `-Wno-deprecated-register`, `-Wno-deprecated-volatile`,
  `-Wno-deprecated-enum-enum-conversion`; clang>=20 adds
  `-Wno-deprecated-literal-operator`, `-Wno-nontrivial-memcall`; clang>=21 adds
  `-Wno-character-conversion`; **aarch64** Clang adds `-Wno-absolute-value`
  (tomlplusplus `abs(log10(double))`).
- **GCC**: `-Wno-unused-result`, `-Wno-volatile`, `-Wno-template-id-cdtor`,
  `-Wno-return-type`, `-Wno-deprecated`, `-fpermissive`.
- **C (all)**: `-Wno-implicit-function-declaration`, `-Wno-incompatible-pointer-types`.
- **Apple**: `-Wno-shorten-64-to-32`, `-Wno-deprecated-declarations`,
  `-Wno-character-conversion`.
- **Android**: `-Wno-switch`, `-Wno-attributes` (plus `_LIBCPP_DISABLE_AVAILABILITY`).

## Adding a target

New `src/` modules must call `xe_target_defaults(<target>)` in their
`CMakeLists.txt` to inherit fatal warnings + include dirs. If a warning must be
suppressed, prefer a narrow per-file `set_source_files_properties` or a guarded
global in the root CMakeLists (with a comment) so it stays consistent across the
matching compiler on every cell.
