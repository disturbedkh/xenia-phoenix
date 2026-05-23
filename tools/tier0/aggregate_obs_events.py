#!/usr/bin/env python3
"""Aggregate obs JSONL — thin wrapper around phoenixctl log summarize."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "phoenixctl"))
from phoenixctl.log_obs import summarize  # noqa: E402
from phoenixctl.paths import find_repo_root  # noqa: E402


def main() -> int:
    p = argparse.ArgumentParser(description="Aggregate obs events JSONL")
    p.add_argument("--title-id", required=True)
    p.add_argument("--telemetry-dir", default="telemetry")
    p.add_argument("--json", action="store_true")
    args = p.parse_args()
    repo = find_repo_root()
    data = summarize(repo, args.title_id, args.telemetry_dir)
    print(json.dumps(data, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
