#!/usr/bin/env python3
"""Classify xenia crash/host log tail into triage buckets (APU, GPU, CPU, Host)."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

GPU_PATTERNS = re.compile(
    r"GPU|D3D|EDRAM|resolve|device\s+lost|readback|shader|texture|present",
    re.IGNORECASE,
)
APU_PATTERNS = re.compile(r"APU|XMA|audio|pcm|voice", re.IGNORECASE)
CPU_PATTERNS = re.compile(
    r"assert|unimplemented|stub|CPU|JIT|guest|exception", re.IGNORECASE
)
HOST_PATTERNS = re.compile(
    r"access violation|segfault|out of memory|OOM|stack overflow|fatal",
    re.IGNORECASE,
)


def score_lines(lines: list[str]) -> dict[str, int]:
    scores = {"GPU": 0, "APU": 0, "CPU": 0, "Host": 0}
    for line in lines:
        if GPU_PATTERNS.search(line):
            scores["GPU"] += 1
        if APU_PATTERNS.search(line):
            scores["APU"] += 1
        if CPU_PATTERNS.search(line):
            scores["CPU"] += 1
        if HOST_PATTERNS.search(line):
            scores["Host"] += 1
    return scores


def recommend(scores: dict[str, int]) -> str:
    if max(scores.values()) == 0:
        return "unknown"
    top = max(scores, key=scores.get)
    # Symptom-led tie-break: APU before GPU when close (audio cut precedes freeze).
    if scores["APU"] > 0 and scores["GPU"] > 0:
        if scores["APU"] >= scores["GPU"] - 1:
            return "APU"
    return top


def main() -> int:
    ap = argparse.ArgumentParser(description="Triage xenia.log tail by keyword buckets.")
    ap.add_argument("log", type=Path, help="Host log file (e.g. telemetry/454107db_crash.log).")
    ap.add_argument(
        "--tail-lines",
        type=int,
        default=200,
        help="Number of lines from end of file to scan.",
    )
    args = ap.parse_args()

    if not args.log.is_file():
        print(f"Missing log: {args.log}", file=sys.stderr)
        return 2

    text = args.log.read_text(encoding="utf-8", errors="replace").splitlines()
    tail = text[-args.tail_lines :] if len(text) > args.tail_lines else text

    scores = score_lines(tail)
    bucket = recommend(scores)

    print(f"Log: {args.log}  (last {len(tail)} lines)")
    print("Bucket scores:", ", ".join(f"{k}={v}" for k, v in sorted(scores.items())))
    print(f"Recommended bucket: {bucket}")

    print("\nLast 15 log lines:")
    for line in tail[-15:]:
        print(line)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
