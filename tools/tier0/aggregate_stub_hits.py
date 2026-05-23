#!/usr/bin/env python3
"""Aggregate kernel_stub_hit_log JSONL files by (module, export)."""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter, defaultdict
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
    ap = argparse.ArgumentParser(description="Summarize kernel stub JSONL logs.")
    ap.add_argument(
        "inputs",
        nargs="*",
        type=Path,
        help="JSONL files or directories (default: tests/kernel_stub/fixtures)",
    )
    ap.add_argument(
        "--out",
        type=Path,
        default=None,
        help="Write summary JSON (default: print table only).",
    )
    ap.add_argument("--top", type=int, default=50, help="Max rows to print.")
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
        default_dir = repo / "tests" / "kernel_stub"
        paths = sorted(default_dir.glob("*.jsonl"))

    if not paths:
        print("No JSONL inputs found.", file=sys.stderr)
        return 1

    counter: Counter[tuple[str, str]] = Counter()
    details: dict[tuple[str, str], set[str]] = defaultdict(set)
    total_lines = 0

    for path in paths:
        for row in load_jsonl(path):
            total_lines += 1
            module = row.get("module", "?")
            export = row.get("export", "?")
            key = (module, export)
            counter[key] += 1
            detail = row.get("detail")
            if detail:
                details[key].add(str(detail)[:120])

    summary = {
        "files": [str(p) for p in paths],
        "total_lines": total_lines,
        "unique_keys": len(counter),
        "hits": [
            {
                "module": m,
                "export": e,
                "count": c,
                "detail_samples": sorted(details[(m, e)])[:5],
            }
            for (m, e), c in counter.most_common()
        ],
    }

    print(f"Files: {len(paths)}  Lines: {total_lines}  Unique: {len(counter)}")
    for i, ((m, e), c) in enumerate(counter.most_common(args.top)):
        print(f"  {c:6d}  {m}  {e}")
        samples = details[(m, e)]
        if samples:
            print(f"         e.g. {next(iter(samples))}")

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(summary, indent=2), encoding="utf-8")
        print(f"Wrote {args.out}")

    return 0 if total_lines == 0 or len(counter) > 0 else 0


if __name__ == "__main__":
    raise SystemExit(main())
