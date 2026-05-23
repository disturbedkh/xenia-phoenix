"""GPU trace validate / dump / observability explain."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any

from phoenixctl.paths import find_repo_root


def validate_trace(trace_path: Path) -> tuple[int, str]:
    repo = find_repo_root()
    script = repo / "tools" / "gpu_replay_ci" / "validate_traces.py"
    if not script.is_file():
        return 1, f"missing {script}"
    proc = subprocess.run(
        [sys.executable, str(script), str(trace_path)],
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    out = (proc.stdout or "") + (proc.stderr or "")
    return proc.returncode, out.strip()


def dump_trace(trace_path: Path, config: str = "Release") -> tuple[int, str]:
    repo = find_repo_root()
    exe = repo / "build" / "bin" / "Windows" / config / "xenia-gpu-d3d12-trace-dump.exe"
    if not exe.is_file():
        return 1, f"missing {exe}"
    proc = subprocess.run(
        [str(exe), str(trace_path)],
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    out = (proc.stdout or "") + (proc.stderr or "")
    return proc.returncode, out.strip()


def _read_jsonl_range(
    events_path: Path, seq_first: int, seq_last: int
) -> list[dict[str, Any]]:
    if not events_path.is_file() or seq_last < seq_first:
        return []
    matched: list[dict[str, Any]] = []
    for line in events_path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            obj = json.loads(line)
        except json.JSONDecodeError:
            continue
        seq = int(obj.get("seq", 0))
        if seq_first <= seq <= seq_last:
            matched.append(obj)
    return matched


def explain_trace(
    trace_path: Path,
    *,
    title_id: str | None = None,
    telemetry_dir: str = "telemetry",
) -> dict[str, Any]:
    """Join ``.xtr.obs.json`` sidecar to ``*_events.jsonl`` invariant window."""
    repo = find_repo_root()
    sidecar_path = Path(str(trace_path) + ".obs.json")
    result: dict[str, Any] = {
        "trace": str(trace_path),
        "sidecar": str(sidecar_path),
        "sidecar_present": sidecar_path.is_file(),
        "events": [],
        "codes": {},
    }
    if not sidecar_path.is_file():
        result["error"] = "missing_obs_sidecar"
        return result

    sidecar = json.loads(sidecar_path.read_text(encoding="utf-8"))
    result["sidecar_body"] = sidecar
    seq_first = int(sidecar.get("obs_event_seq_first", 0))
    seq_last = int(sidecar.get("obs_event_seq_last", 0))

    tid = title_id
    if not tid and sidecar.get("session"):
        tid = None
    if not tid:
        # Infer from trace parent folder name: telemetry/{tid}_gpu_trace/foo.xtr
        parent = trace_path.parent.name
        if parent.endswith("_gpu_trace"):
            tid = parent.replace("_gpu_trace", "")

    if tid:
        events_path = repo / telemetry_dir / f"{tid.lower().replace('0x', '')}_events.jsonl"
        result["events_path"] = str(events_path)
        events = _read_jsonl_range(events_path, seq_first, seq_last)
        result["events"] = events
        codes: dict[str, int] = {}
        for ev in events:
            code = str(ev.get("code", ""))
            if code:
                codes[code] = codes.get(code, 0) + 1
        result["codes"] = codes
    else:
        result["error"] = "title_id_unknown"

    return result
