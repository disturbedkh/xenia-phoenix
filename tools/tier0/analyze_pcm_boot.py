#!/usr/bin/env python3
"""Analyze first N seconds of apu_pcm_hash_log JSONL (boot / launch audio profile)."""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path


def load(path: Path) -> list[dict]:
    rows: list[dict] = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line:
            continue
        rows.append(json.loads(line))
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Analyze PCM hash JSONL boot windows (launch audio)."
    )
    ap.add_argument("pcm", type=Path, help="PCM hash JSONL capture.")
    ap.add_argument(
        "--boot-sec",
        type=int,
        default=120,
        help="Analyze first N seconds after the first window (by window_start_ms).",
    )
    args = ap.parse_args()

    if not args.pcm.is_file():
        print(f"Missing PCM log: {args.pcm}", file=sys.stderr)
        return 2

    rows = load(args.pcm)
    if not rows:
        print("PCM log is empty")
        return 1

    rows.sort(key=lambda r: int(r.get("window_start_ms", 0)))
    first_ms = int(rows[0].get("window_start_ms", 0))
    cutoff = first_ms + args.boot_sec * 1000
    boot = [r for r in rows if int(r.get("window_start_ms", 0)) < cutoff]
    if not boot:
        boot = rows[: min(len(rows), args.boot_sec)]

    hashes = [r.get("sha256_pcm_mix", "") for r in boot]
    counts = Counter(hashes)
    unique = len(set(hashes))
    dominant_hash, dominant_n = counts.most_common(1)[0]

    title_ids = Counter(r.get("title_id", "") for r in boot)

    print(f"PCM file: {args.pcm}")
    print(f"Total windows: {len(rows)}  first window: {first_ms} ms")
    print(f"Boot: first {args.boot_sec}s -> {len(boot)} windows (until {cutoff} ms)")
    print(f"Boot unique hashes: {unique}")
    print(f"Boot dominant hash: {dominant_n}/{len(boot)} windows")
    print(f"Title IDs in boot segment: {dict(title_ids)}")

    print("First 5 boot windows:")
    for r in boot[:5]:
        print(
            f"  {r.get('window_start_ms')} ms  "
            f"title={r.get('title_id')}  {r.get('sha256_pcm_mix', '')[:16]}..."
        )

    if unique <= 3:
        print("NOTE: very low hash diversity in boot window (possible flatline / stuck mix)")
    elif unique > len(boot) * 0.8:
        print("NOTE: high hash churn in boot window (active audio, not silent)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
