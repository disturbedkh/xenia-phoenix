"""Tail and aggregate telemetry files."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

from phoenixctl.paths import find_repo_root, title_paths

TAIL_KINDS = ("stubs", "xma", "pcm", "crash")


def _path_for_kind(paths, kind: str) -> Path:
    if kind == "stubs":
        return paths.stubs
    if kind == "xma":
        return paths.xma
    if kind == "pcm":
        return paths.pcm
    if kind == "crash":
        return paths.crash
    raise ValueError(f"Unknown kind: {kind!r}. Use: stubs, xma, pcm, crash")


def tail_lines(title_id: str, kind: str, lines: int = 20, telemetry_dir: str = "telemetry") -> dict:
    repo = find_repo_root()
    paths = title_paths(repo, title_id, telemetry_dir)
    path = _path_for_kind(paths, kind)
    if not path.is_file():
        return {
            "kind": kind,
            "path": paths.as_dict(repo)[kind if kind != "crash" else "crash"],
            "lines": [],
            "exists": False,
        }
    all_lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    tail = all_lines[-lines:] if lines > 0 else all_lines
    return {
        "kind": kind,
        "path": paths.as_dict(repo)[
            {"stubs": "stubs", "xma": "xma", "pcm": "pcm", "crash": "crash"}[kind]
        ],
        "lines": tail,
        "total_lines": len(all_lines),
        "exists": True,
    }


def aggregate_stubs(title_id: str, telemetry_dir: str = "telemetry") -> tuple[int, dict | str]:
    repo = find_repo_root()
    paths = title_paths(repo, title_id, telemetry_dir)
    if not paths.stubs.is_file():
        return 2, {"error": "stub log missing", "path": paths.as_dict(repo)["stubs"]}
    script = repo / "tools" / "tier0" / "aggregate_stub_hits.py"
    out_path = paths.stub_summary
    proc = subprocess.run(
        [sys.executable, str(script), str(paths.stubs), "--out", str(out_path)],
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if proc.returncode != 0:
        return proc.returncode, {"stderr": proc.stderr, "stdout": proc.stdout}
    if out_path.is_file():
        return 0, json.loads(out_path.read_text(encoding="utf-8"))
    return 0, {"stdout": proc.stdout}


def compare_pcm(
    title_id: str,
    baseline: str | None = None,
    telemetry_dir: str = "telemetry",
) -> tuple[int, str]:
    repo = find_repo_root()
    paths = title_paths(repo, title_id, telemetry_dir)
    base = Path(baseline) if baseline else paths.pcm_baseline
    script = repo / "tools" / "tier0" / "compare_pcm_hash_log.py"
    proc = subprocess.run(
        [sys.executable, str(script), str(paths.pcm), str(base)],
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return proc.returncode, (proc.stdout or "") + (proc.stderr or "")


def list_patches(title_id: str) -> tuple[int, str]:
    repo = find_repo_root()
    script = repo / "tools" / "tier0" / "list_smoke_patches.py"
    proc = subprocess.run(
        [sys.executable, str(script), "--title-id", title_id.upper()],
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return proc.returncode, (proc.stdout or "") + (proc.stderr or "")
