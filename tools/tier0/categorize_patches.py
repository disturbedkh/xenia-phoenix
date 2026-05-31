#!/usr/bin/env python3
"""
Tier 0 (MVP3): heuristic categorization of xenia-canary/game-patches TOML files.

Clone: https://github.com/xenia-canary/game-patches
Set GAME_PATCHES_ROOT to that directory, or place it as a sibling folder named
`game-patches` next to this xenia repo.
"""

from __future__ import annotations

import argparse
import json
import os
import re
from pathlib import Path
from typing import Any

try:
    import tomllib  # py3.11+
except ImportError:
    tomllib = None  # type: ignore


def load_toml(path: Path) -> dict[str, Any]:
    if tomllib is None:
        raise SystemExit("Python 3.11+ required (tomllib).")
    return tomllib.loads(path.read_text(encoding="utf-8"))


def guess_category(text: str) -> str:
    t = text.lower()
    if any(
        k in t
        for k in (
            "drm",
            "check",
            "media id",
            "anti",
            "tamper",
            "xcrypt",
            "devkit",
        )
    ):
        return "A"
    if any(
        k in t
        for k in (
            "intro",
            "skip",
            "fps",
            "uncap",
            "widescreen",
            "fov",
            "language",
            "qol",
            "resolution",
            "720",
            "1080",
            "motion blur",
            "aspect ratio",
            "21:9",
            "ultrawide",
            "anisotropic",
        )
    ):
        return "B"
    if any(
        k in t
        for k in (
            "hang",
            "crash",
            "softlock",
            "deadlock",
            "flicker",
            "black screen",
            "green",
            "tiled",
            "edram",
            "audio",
            "xma",
            "stutter",
            "save",
            "assert",
        )
    ):
        return "D"
    return "C"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--patches-root",
        type=Path,
        default=None,
        help="Path to game-patches repo (or set GAME_PATCHES_ROOT).",
    )
    ap.add_argument("--out", type=Path, default=Path("patch_debt_dashboard_data.json"))
    args = ap.parse_args()

    root = args.patches_root or os.environ.get("GAME_PATCHES_ROOT")
    if root:
        root = Path(root)
    else:
        guess = Path(__file__).resolve().parents[2].parent / "game-patches"
        root = guess if guess.is_dir() else None

    if not root or not root.is_dir():
        print(
            "No game-patches directory found. Clone xenia-canary/game-patches and set\n"
            "  GAME_PATCHES_ROOT\n"
            "or pass --patches-root.\n"
            "Writing empty sample output."
        )
        args.out.write_text(
            json.dumps(
                {
                    "note": "Run with game-patches checkout for real data.",
                    "counts_by_category": {},
                },
                indent=2,
            ),
            encoding="utf-8",
        )
        return 0

    counts: dict[str, int] = {}
    rows: list[dict[str, Any]] = []
    for path in sorted(root.rglob("*.patch.toml")):
        try:
            data = load_toml(path)
        except Exception as e:  # noqa: BLE001
            rows.append({"file": str(path), "error": str(e)})
            continue
        title = str(data.get("title_name", ""))
        patches = data.get("patch", [])
        if not isinstance(patches, list):
            patches = []
        for p in patches:
            if not isinstance(p, dict):
                continue
            name = str(p.get("name", ""))
            desc = str(p.get("desc", ""))
            blob = f"{name} {desc} {title}"
            cat = guess_category(blob)
            counts[cat] = counts.get(cat, 0) + 1
            rows.append(
                {
                    "file": path.name,
                    "title": title,
                    "patch": name,
                    "category_guess": cat,
                }
            )

    top = sorted(counts.items(), key=lambda kv: kv[1], reverse=True)[:10]
    out = {
        "patches_root": str(root),
        "counts_by_category": counts,
        "top_categories": top,
        "rows": rows[:5000],
    }
    args.out.write_text(json.dumps(out, indent=2), encoding="utf-8")
    print(f"Wrote {args.out} ({len(rows)} patch entries sampled, capped in JSON).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
