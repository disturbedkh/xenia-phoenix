#!/usr/bin/env python3
"""List category C and D patches for smoke title IDs (gameplay triage)."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


def normalize_title_id(raw: str) -> str:
    s = raw.strip().upper().replace("0X", "")
    if not re.fullmatch(r"[0-9A-F]{8}", s):
        raise ValueError(f"invalid title_id: {raw!r} (expected 8 hex digits)")
    return s


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Emit C/D patch rows from patch_debt_dashboard_data.json for smoke titles."
    )
    ap.add_argument(
        "--title-id",
        action="append",
        dest="title_ids",
        required=True,
        help="8-digit hex title id (e.g. 4D5307D1 or 0x4D5307D1); repeat per title.",
    )
    ap.add_argument(
        "--dashboard",
        type=Path,
        default=Path("docs/patch_debt_dashboard_data.json"),
        help="JSON from categorize_patches.py",
    )
    ap.add_argument(
        "--categories",
        default="C,D",
        help="Comma-separated categories to include (default: C,D).",
    )
    ap.add_argument("--json-out", type=Path, default=None, help="Write filtered rows as JSON.")
    args = ap.parse_args()

    if not args.dashboard.is_file():
        print(f"Missing {args.dashboard}", file=sys.stderr)
        return 1

    data = json.loads(args.dashboard.read_text(encoding="utf-8"))
    rows = data.get("rows") or []
    want_cats = {c.strip().upper() for c in args.categories.split(",") if c.strip()}
    title_ids = [normalize_title_id(t) for t in args.title_ids]

    matched: list[dict] = []
    for row in rows:
        if not isinstance(row, dict):
            continue
        cat = str(row.get("category_guess", "")).upper()
        if cat not in want_cats:
            continue
        fname = str(row.get("file", ""))
        prefix = fname.split(" ", 1)[0].upper().replace("0X", "")
        if prefix not in title_ids:
            continue
        matched.append(row)

    print(f"Smoke titles: {', '.join(title_ids)}")
    print(f"Categories: {','.join(sorted(want_cats))}")
    print(f"Matches: {len(matched)} patch rows\n")

    by_file: dict[str, list[dict]] = {}
    for row in matched:
        by_file.setdefault(str(row.get("file", "")), []).append(row)

    for fname in sorted(by_file):
        print(f"## {fname}")
        for row in by_file[fname]:
            print(
                f"  [{row.get('category_guess')}] {row.get('patch', '')} "
                f"— {row.get('title', '')}"
            )
        print()

    if args.json_out:
        out = {
            "title_ids": title_ids,
            "categories": sorted(want_cats),
            "count": len(matched),
            "rows": matched,
        }
        args.json_out.write_text(json.dumps(out, indent=2), encoding="utf-8")
        print(f"Wrote {args.json_out}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
