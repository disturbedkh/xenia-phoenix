#!/usr/bin/env python3
"""Aggregate apu_xma_divergence_log JSONL by (site, detail prefix)."""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path


def load_jsonl(path: Path) -> list[dict]:
    rows: list[dict] = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line:
            continue
        rows.append(json.loads(line))
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description="Summarize APU XMA divergence JSONL.")
    ap.add_argument("inputs", nargs="*", type=Path, help="JSONL files or directories.")
    ap.add_argument("--out", type=Path, default=None, help="Write summary JSON.")
    ap.add_argument("--top", type=int, default=50)
    args = ap.parse_args()

    repo = Path(__file__).resolve().parents[2]
    paths: list[Path] = []
    if args.inputs:
        for inp in args.inputs:
            if inp.is_dir():
                paths.extend(sorted(inp.glob("*.jsonl")))
            elif inp.is_file():
                paths.append(inp)
    else:
        paths = sorted((repo / "telemetry").glob("*_xma.jsonl"))

    if not paths:
        print("No JSONL inputs found.", file=sys.stderr)
        return 1

    counter: Counter[tuple[str, str]] = Counter()
    total = 0
    for path in paths:
        for row in load_jsonl(path):
            total += 1
            site = row.get("site", "?")
            detail = row.get("detail", "")
            counter[(site, detail[:80])] += 1

    rows = counter.most_common(args.top)
    print(f"lines={total} unique={len(counter)}")
    for (site, detail), count in rows:
        print(f"{count:6d}  {site}  {detail}")

    if args.out:
        payload = {
            "total_lines": total,
            "top": [
                {"site": s, "detail": d, "count": c} for (s, d), c in rows
            ],
        }
        args.out.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    return 0 if total == 0 else 0


if __name__ == "__main__":
    raise SystemExit(main())
