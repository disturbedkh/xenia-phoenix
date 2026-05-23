"""Session state in telemetry/.phoenix_session.json."""

from __future__ import annotations

import json
import os
import signal
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from phoenixctl.paths import TitlePaths, find_repo_root


def _iso_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def read_session(path: Path) -> dict[str, Any] | None:
    if not path.is_file():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return None


def write_session(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2), encoding="utf-8")


def clear_session(path: Path) -> None:
    if path.is_file():
        path.unlink()


def pid_alive(pid: int) -> bool:
    if pid <= 0:
        return False
    if sys.platform == "win32":
        try:
            os.kill(pid, 0)
        except OSError:
            return False
        return True
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return True


def redact_game_path(game: str | None) -> str | None:
    if not game:
        return None
    return Path(game).name


def start_session_record(
    paths: TitlePaths,
    *,
    pid: int,
    mode: str,
    game_path: str | None = None,
    xenia_exe: str | None = None,
) -> dict[str, Any]:
    repo = find_repo_root()
    rel_paths = paths.as_dict(repo)
    data: dict[str, Any] = {
        "pid": pid,
        "title_id": paths.title_id,
        "mode": mode,
        "started_at": _iso_now(),
        "telemetry_dir": rel_paths["telemetry_dir"],
        "paths": {
            "stubs": rel_paths["stubs"],
            "xma": rel_paths["xma"],
            "pcm": rel_paths["pcm"],
            "crash": rel_paths["crash"],
            "gpu_trace": rel_paths["gpu_trace"],
        },
        "game_basename": redact_game_path(game_path),
        "xenia_exe": xenia_exe,
    }
    write_session(paths.session_file, data)
    return data


def session_status(paths: TitlePaths) -> dict[str, Any]:
    data = read_session(paths.session_file)
    if not data:
        return {"active": False, "reason": "no_session_file"}
    pid = int(data.get("pid", 0))
    alive = pid_alive(pid)
    return {
        "active": alive,
        "pid": pid,
        "pid_alive": alive,
        "title_id": data.get("title_id"),
        "mode": data.get("mode"),
        "started_at": data.get("started_at"),
        "paths": data.get("paths"),
        "game_basename": data.get("game_basename"),
    }


def read_session_title_from_repo(repo: Path | None = None) -> str | None:
    from phoenixctl.paths import find_repo_root

    root = repo or find_repo_root()
    p = root / "telemetry" / ".phoenix_session.json"
    data = read_session(p)
    if not data:
        return None
    tid = data.get("title_id")
    return str(tid) if tid else None


def kill_session(paths: TitlePaths) -> dict[str, Any]:
    data = read_session(paths.session_file)
    if not data:
        return {"killed": False, "reason": "no_session"}
    pid = int(data.get("pid", 0))
    killed = False
    if pid_alive(pid):
        if sys.platform == "win32":
            subprocess.run(
                ["taskkill", "/PID", str(pid), "/F"],
                check=False,
                capture_output=True,
            )
        else:
            try:
                os.kill(pid, signal.SIGTERM)
            except OSError:
                pass
        killed = True
    clear_session(paths.session_file)
    return {"killed": killed, "pid": pid}
