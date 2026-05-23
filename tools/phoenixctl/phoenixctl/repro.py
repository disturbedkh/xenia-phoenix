"""GPU repro capture — discovery to proof-loop handoff."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

from phoenixctl.paths import find_repo_root


def _powershell_exe() -> list[str]:
    if sys.platform == "win32":
        return ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File"]
    return ["pwsh", "-NoProfile", "-File"]


def repro_capture(
    title_id: str,
    *,
    game: str | None = None,
    config: str = "Debug",
    telemetry_dir: str = "telemetry",
    launch: bool = False,
    post_only: bool = False,
    gfx_workaround: bool = False,
    release_menu: bool = False,
    poll_sec: int = 5,
    poll_count: int = 6,
    trace_path: str | None = None,
    run_replay: bool = False,
) -> tuple[int, dict]:
    repo = find_repo_root()
    script = repo / "tools" / "tier0" / "gpu_repro_capture.ps1"
    if not script.is_file():
        return 1, {"error": f"missing {script}"}

    args = [
        "-TitleId",
        title_id.upper(),
        "-Config",
        config,
        "-TelemetryDir",
        telemetry_dir,
        "-PollSec",
        str(poll_sec),
        "-PollCount",
        str(poll_count),
    ]
    if game:
        args += ["-GamePath", game]
    if launch:
        args.append("-Launch")
    if post_only:
        args.append("-PostOnly")
    if gfx_workaround:
        args.append("-GfxWorkaround")
    if release_menu:
        args.append("-ReleaseMenu")
    if trace_path:
        args += ["-TracePath", trace_path]
    if run_replay:
        args.append("-RunReplay")

    proc = subprocess.run(
        _powershell_exe() + [str(script)] + args,
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    stdout = (proc.stdout or "").strip()
    stderr = (proc.stderr or "").strip()
    try:
        data = json.loads(stdout) if stdout else {}
    except json.JSONDecodeError:
        data = {"raw_stdout": stdout, "stderr": stderr}
    if stderr:
        data["stderr"] = stderr
    data["exit_code"] = proc.returncode
    return proc.returncode, data
