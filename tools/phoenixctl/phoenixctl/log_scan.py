"""Crash log scanning for triage buckets."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

from phoenixctl.paths import find_repo_root, title_paths


def scan_crash_log(title_id: str, telemetry_dir: str = "telemetry") -> dict:
    repo = find_repo_root()
    paths = title_paths(repo, title_id, telemetry_dir)
    log_path = paths.crash
    result: dict = {
        "log": str(log_path.relative_to(repo)) if log_path.is_file() else None,
        "exists": log_path.is_file(),
        "upload_range_errors": 0,
        "bucket": "unknown",
        "triage_script": {},
    }
    if not log_path.is_file():
        return result

    text = log_path.read_text(encoding="utf-8", errors="replace")
    result["upload_range_errors"] = len(
        re.findall(r"Invalid upload range for GPU", text)
    )

    script = repo / "tools" / "tier0" / "triage_crash_log.py"
    if script.is_file():
        proc = subprocess.run(
            [sys.executable, str(script), str(log_path), "--tail-lines", "200"],
            cwd=str(repo),
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        out = (proc.stdout or "") + (proc.stderr or "")
        result["triage_script"] = {
            "exit_code": proc.returncode,
            "output": out.strip(),
        }
        m = re.search(r"Recommended bucket:\s*(\w+)", out)
        if m:
            result["bucket"] = m.group(1)

    return result
