#!/usr/bin/env python3
"""Single source of truth for Phoenix build directory paths.

Layout: Build/<OS>/<arch>/<Config>/  (Android omits <arch>).

Importable by xenia-build.py; also runnable as a CLI for ps1/sh/CI:
  python tools/build/xenia_paths.py bin --config Release [--target-arch arm64]
  python tools/build/xenia_paths.py build-dir [--target-arch arm64] [--os Android]
"""
from __future__ import annotations

import argparse
import platform
import sys

BUILD_ROOT = "Build"

# OS names that always include an arch segment in the path.
_MULTI_ARCH_OS = frozenset({"Windows", "Linux", "macOS"})

# Canonical arch folder names under Build/<OS>/.
_ARCH_DIR = {
    "x64": "x64",
    "arm64": "ARM64",
}

# Map platform.system() / explicit --os to the folder name under Build/.
_OS_FROM_SYS = {
    "Windows": "Windows",
    "Linux": "Linux",
    "Darwin": "macOS",
}


def normalize_target_arch(value: str | None) -> str | None:
    """Normalize architecture aliases to 'x64' or 'arm64'."""
    if value is None:
        return None
    v = value.lower()
    if v in ("arm64", "aarch64", "a64"):
        return "arm64"
    if v in ("x64", "x86_64", "amd64", "x86"):
        return "x64"
    raise ValueError(
        f"unknown architecture '{value}' (expected: arm64, x64, amd64, x86_64)"
    )


def native_arch() -> str:
    """Return native host architecture ('arm64' or 'x64')."""
    machine = platform.machine()
    if machine in ("ARM64", "aarch64"):
        return "arm64"
    return "x64"


def detect_os(os_name: str | None = None) -> str:
    """Detect or validate the OS folder name."""
    if os_name:
        name = os_name
        if name == "Android":
            return "Android"
        if name in _MULTI_ARCH_OS:
            return name
        if name == "Darwin":
            return "macOS"
        raise ValueError(f"unknown OS '{os_name}'")
    if sys.platform == "win32":
        return "Windows"
    if sys.platform == "darwin":
        return "macOS"
    # Linux or Android cross-build host
    return "Linux"


def effective_arch(os_name: str, target_arch: str | None) -> str | None:
    """Return the arch segment for *os_name*, or None when omitted (Android)."""
    if os_name == "Android":
        return None
    if os_name not in _MULTI_ARCH_OS:
        return None
    return normalize_target_arch(target_arch) or native_arch()


def get_build_dir(
    os_name: str | None = None,
    target_arch: str | None = None,
) -> str:
    """Return the CMake binary directory, e.g. Build/Windows/x64."""
    os_folder = detect_os(os_name)
    arch = effective_arch(os_folder, target_arch)
    if arch is None:
        return f"{BUILD_ROOT}/{os_folder}"
    arch_dir = _ARCH_DIR[arch]
    return f"{BUILD_ROOT}/{os_folder}/{arch_dir}"


def normalize_config(config: str) -> str:
    """Title-case a build configuration name."""
    c = config.strip()
    if not c:
        raise ValueError("config must not be empty")
    return c[0].upper() + c[1:].lower()


def get_bin_dir(
    os_name: str | None = None,
    target_arch: str | None = None,
    config: str = "Release",
) -> str:
    """Return the directory containing built binaries for *config*."""
    return f"{get_build_dir(os_name, target_arch)}/{normalize_config(config)}"


def legacy_build_dirs() -> list[str]:
    """Retired top-level build directories (pre-Build/ consolidation)."""
    return [
        "build",
        "build-arm64",
        "build-x64",
        "build-android-arm64",
        "build-windows",
    ]


def all_build_dirs() -> list[str]:
    """All build output dirs including the current Build/ root (for nuke)."""
    return legacy_build_dirs() + [BUILD_ROOT]


def _cli() -> int:
    parser = argparse.ArgumentParser(description="Resolve Phoenix build paths.")
    sub = parser.add_subparsers(dest="command", required=True)

    common = argparse.ArgumentParser(add_help=False)
    common.add_argument(
        "--os",
        default=None,
        help="OS folder (Windows, Linux, macOS, Android). Auto-detected if omitted.",
    )
    common.add_argument(
        "--target-arch",
        default=None,
        help="Target architecture (x64, arm64). Defaults to native host.",
    )

    p_bin = sub.add_parser("bin", parents=[common], help="Print bin/<Config> path.")
    p_bin.add_argument(
        "--config", default="Release", help="Build configuration (Release, Debug, Checked)."
    )

    sub.add_parser(
        "build-dir", parents=[common], help="Print CMake binary directory."
    )

    args = parser.parse_args()
    try:
        if args.command == "bin":
            print(get_bin_dir(args.os, args.target_arch, args.config))
        elif args.command == "build-dir":
            print(get_build_dir(args.os, args.target_arch))
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(_cli())
