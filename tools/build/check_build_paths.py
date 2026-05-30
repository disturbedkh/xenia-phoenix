#!/usr/bin/env python3
"""Guard against retired Phoenix build path references in the repo."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

SKIP_DIRS = {
    "Build",
    "build",
    "build-arm64",
    "build-android-arm64",
    "build-windows",
    "build-x64",
    "third_party",
    ".git",
    "node_modules",
    "__pycache__",
    "telemetry",
}

SKIP_SUFFIXES = {".log", ".png", ".jpg", ".exe", ".dll", ".so", ".xtr", ".bin", ".o"}

SKIP_FILES = {
    "check_build_paths.py",
    "xenia_paths.py",
    "xenia-build.py",
    ".gitignore",
}

# Case-sensitive patterns (avoid matching Build/Windows).
RETIRED = [
    re.compile(r"build/bin/Windows"),
    re.compile(r"build\\bin\\Windows"),
    re.compile(r"build/bin/Linux"),
    re.compile(r"build/bin/macOS"),
    re.compile(r"build-android-arm64"),
    re.compile(r'["\']build-arm64["\']'),
    re.compile(r"-B build-android"),
    re.compile(r"cmake --build build[^/\\]"),
    re.compile(r'["\']build/bin/'),
]


def iter_source_files() -> list[Path]:
    out: list[Path] = []
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if path.name in SKIP_FILES:
            continue
        if any(part in SKIP_DIRS for part in path.parts):
            continue
        if path.suffix.lower() in SKIP_SUFFIXES:
            continue
        out.append(path)
    return out


def main() -> int:
    violations: list[str] = []
    for path in iter_source_files():
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        rel = path.relative_to(ROOT).as_posix()
        for pat in RETIRED:
            for match in pat.finditer(text):
                line = text.count("\n", 0, match.start()) + 1
                violations.append(f"{rel}:{line}: {match.group(0)!r}")

    if violations:
        print("Retired build path references found:", file=sys.stderr)
        for v in sorted(violations):
            print(f"  {v}", file=sys.stderr)
        return 1

    print("check_build_paths: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
