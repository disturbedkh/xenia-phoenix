#!/usr/bin/env python3
"""Compare apu_pcm_hash_log JSONL against a baseline file."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def load(path: Path) -> list[dict]:
    rows: list[dict] = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line:
            continue
        rows.append(json.loads(line))
    return rows


def index_by_window(rows: list[dict]) -> dict[int, str]:
    out: dict[int, str] = {}
    for row in rows:
        start = int(row.get("window_start_ms", 0))
        out[start] = row.get("sha256_pcm_mix", "")
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Diff PCM hash JSONL vs baseline.")
    ap.add_argument("current", type=Path, help="New capture JSONL.")
    ap.add_argument("baseline", type=Path, help="Baseline JSONL.")
    ap.add_argument(
        "--update-baseline",
        action="store_true",
        help="Copy current over baseline and exit 0.",
    )
    args = ap.parse_args()

    if not args.current.is_file():
        print(f"Missing current log: {args.current}", file=sys.stderr)
        return 2
    if args.update_baseline:
        if not args.current.is_file():
            return 2
        args.baseline.write_text(
            args.current.read_text(encoding="utf-8"), encoding="utf-8"
        )
        print(f"Updated baseline: {args.baseline}")
        return 0
    if not args.baseline.is_file():
        print(f"Missing baseline: {args.baseline}", file=sys.stderr)
        print("Run once with --update-baseline to establish.", file=sys.stderr)
        return 2

    cur = index_by_window(load(args.current))
    base = index_by_window(load(args.baseline))

    drift = 0
    for start, hash_val in sorted(cur.items()):
        expected = base.get(start)
        if expected is None:
            print(f"new window {start} ms (no baseline row)")
            drift += 1
            continue
        if expected != hash_val:
            print(f"drift at {start} ms: {expected} -> {hash_val}")
            drift += 1

    for start in sorted(set(base) - set(cur)):
        print(f"missing window {start} ms in current capture")
        drift += 1

    if drift:
        print(f"{drift} window(s) differ")
        return 1
    print("PCM hash log matches baseline")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
