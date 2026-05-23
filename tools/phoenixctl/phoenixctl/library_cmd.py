"""Game library commands for phoenixctl."""

from __future__ import annotations

import argparse
from pathlib import Path

from phoenixctl.paths import find_repo_root


def _storage_root(repo: Path) -> Path:
    return repo / "xenia-phoenix-src" / "build" / "bin" / "Release"


def cmd_library_scan(args: argparse.Namespace) -> int:
    """Placeholder: scan is performed in-emulator; prints watched dirs hint."""
    repo = find_repo_root()
    lib_path = _storage_root(repo) / "library.toml"
    print(f"Library file: {lib_path}")
    print("Use the Phoenix launcher (F9) > Add folder to scan, or edit library.toml.")
    return 0


def cmd_library_list(args: argparse.Namespace) -> int:
    repo = find_repo_root()
    lib_path = _storage_root(repo) / "library.toml"
    if not lib_path.is_file():
        print("No library.toml found. Launch xenia_canary and add folders first.")
        return 1
    print(lib_path.read_text(encoding="utf-8"))
    return 0


def register_library_subparser(sub: argparse._SubParsersAction) -> None:
    p_lib = sub.add_parser("library", help="Game library (library.toml)")
    lib_sub = p_lib.add_subparsers(dest="library_cmd", required=True)
    p_scan = lib_sub.add_parser("scan", help="Rescan watched folders (see GUI)")
    p_scan.set_defaults(func=cmd_library_scan)
    p_list = lib_sub.add_parser("list", help="Print library.toml")
    p_list.set_defaults(func=cmd_library_list)
