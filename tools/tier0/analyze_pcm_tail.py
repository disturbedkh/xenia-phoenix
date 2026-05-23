#!/usr/bin/env python3
"""Analyze trailing windows of apu_pcm_hash_log JSONL for crash-leading audio signals."""

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
    ap = argparse.ArgumentParser(description="Analyze PCM hash JSONL tail windows.")
    ap.add_argument("pcm", type=Path, help="PCM hash JSONL capture.")
    ap.add_argument(
        "--tail-sec",
        type=int,
        default=120,
        help="Analyze last N seconds of windows (by window_start_ms).",
    )
    ap.add_argument(
        "--silence-hash",
        type=str,
        default="",
        help="Optional hash value treated as silence (repeat count reported).",
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
    last_ms = int(rows[-1].get("window_start_ms", 0))
    cutoff = max(0, last_ms - args.tail_sec * 1000)
    tail = [r for r in rows if int(r.get("window_start_ms", 0)) >= cutoff]
    if not tail:
        tail = rows[-min(len(rows), args.tail_sec) :]

    hashes = [r.get("sha256_pcm_mix", "") for r in tail]
    unique = set(hashes)
    counts = Counter(hashes)

    print(f"PCM file: {args.pcm}")
    print(f"Total windows: {len(rows)}  span: {rows[0].get('window_start_ms')} .. {last_ms} ms")
    print(f"Tail: last {args.tail_sec}s -> {len(tail)} windows (from {cutoff} ms)")
    print(f"Tail unique hashes: {len(unique)}")

    # Dominant hash in tail (stability / flatline hint).
    dominant_hash, dominant_n = counts.most_common(1)[0]
    print(f"Tail dominant hash: {dominant_n}/{len(tail)} windows")

    # Last 3 windows (approx 1-3s at 1s interval) for audio-cut hypothesis.
    print("Last 3 windows:")
    for r in rows[-3:]:
        start = r.get("window_start_ms")
        h = r.get("sha256_pcm_mix", "")
        print(f"  {start} ms  {h[:16]}...")

    # First tail window where hash differs from previous stable (session) hash.
    if len(rows) > len(tail):
        pre_tail = rows[: len(rows) - len(tail)]
        pre_dominant = Counter(r.get("sha256_pcm_mix", "") for r in pre_tail).most_common(1)
        if pre_dominant:
            stable_hash = pre_dominant[0][0]
            for r in tail:
                h = r.get("sha256_pcm_mix", "")
                if h != stable_hash:
                    print(
                        f"First tail drift from pre-tail dominant: "
                        f"{r.get('window_start_ms')} ms"
                    )
                    break
            else:
                print("No hash drift vs pre-tail dominant in tail segment")

    if args.silence_hash:
        silent = sum(1 for h in hashes if h == args.silence_hash)
        print(f"Silence hash matches in tail: {silent}/{len(tail)}")

    # Heuristic: single hash in last 2 windows -> possible audio cut before freeze.
    last2 = hashes[-2:] if len(hashes) >= 2 else hashes
    if len(set(last2)) == 1:
        print("NOTE: last 2 windows share one hash (possible audio flatline before freeze)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
