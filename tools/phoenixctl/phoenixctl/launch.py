"""Launch wrappers for tier0 PowerShell scripts."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

from phoenixctl.config import resolve_game_path
from phoenixctl.launch_guard import verify_instrumented_launch
from phoenixctl.paths import TitlePaths, find_repo_root, title_paths
from phoenixctl.session import start_session_record


def _powershell_exe() -> list[str]:
    if sys.platform == "win32":
        return [
            "powershell",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
        ]
    return ["pwsh", "-NoProfile", "-File"]


def _run_ps1(script: Path, args: list[str], repo: Path) -> subprocess.CompletedProcess[str]:
    cmd = _powershell_exe() + [str(script)] + args
    return subprocess.run(
        cmd,
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )


def _default_xenia_exe(config: str) -> str:
    repo = find_repo_root()
    proc = subprocess.run(
        [sys.executable, str(repo / "tools" / "build" / "xenia_paths.py"),
         "bin", "--config", config],
        cwd=str(repo),
        capture_output=True,
        text=True,
        check=True,
    )
    return str(Path(proc.stdout.strip()) / "xenia_canary.exe")


def preflight(config: str = "Release") -> tuple[int, str]:
    repo = find_repo_root()
    script = repo / "tools" / "tier0" / "smoke_session_ready.ps1"
    proc = _run_ps1(script, ["-Config", config], repo)
    out = (proc.stdout or "") + (proc.stderr or "")
    return proc.returncode, out


def launch_smoke(
    title_id: str,
    game: str | None,
    *,
    duration: int = 300,
    fresh: bool = False,
    config: str = "Release",
    telemetry_dir: str = "telemetry",
) -> tuple[int, dict]:
    repo = find_repo_root()
    game_path = resolve_game_path(title_id, game, repo)
    paths = title_paths(repo, title_id, telemetry_dir)
    script = repo / "tools" / "tier0" / "run_smoke_capture.ps1"
    args = [
        "-TitleId",
        title_id.upper(),
        "-GamePath",
        game_path,
        "-DurationSec",
        str(duration),
        "-TelemetryDir",
        telemetry_dir,
        "-XeniaExe",
        _default_xenia_exe(config),
    ]
    proc = _run_ps1(script, args, repo)
    result = {
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "paths": paths.as_dict(repo),
        "game_basename": Path(game_path).name,
    }
    return proc.returncode, result


def launch_triage(
    title_id: str,
    game: str | None,
    *,
    fresh: bool = False,
    duration: int = 0,
    config: str = "Auto",
    telemetry_dir: str = "telemetry",
    capture_pid: bool = True,
) -> tuple[int, dict]:
    repo = find_repo_root()
    game_path = resolve_game_path(title_id, game, repo)
    paths = title_paths(repo, title_id, telemetry_dir)

    if duration > 0:
        script = repo / "tools" / "tier0" / "launch_bf2_triage.ps1"
        args = [
            "-GamePath",
            game_path,
            "-TitleId",
            title_id.upper(),
            "-DurationSec",
            str(duration),
            "-TelemetryDir",
            telemetry_dir,
            "-Configuration",
            config,
        ]
        if fresh:
            args.append("-FreshSession")
        proc = _run_ps1(script, args, repo)
        return proc.returncode, {
            "exit_code": proc.returncode,
            "mode": "smoke_timed",
            "stdout": proc.stdout,
            "stderr": proc.stderr,
            "paths": paths.as_dict(repo),
        }

    # Windows: subprocess.Popen argv does not reach xenia (GetCommandLineW path).
    # Use launch_bf2_triage.ps1 Start-Process quoting (same as manual BF2 workflow).
    paths.telemetry_dir.mkdir(parents=True, exist_ok=True)
    paths.gpu_trace.mkdir(parents=True, exist_ok=True)

    if not Path(game_path).exists():
        return 1, {"error": f"Missing game path: {game_path}"}

    ps_config = config if config in ("Debug", "Release", "Auto") else "Auto"
    script = repo / "tools" / "tier0" / "launch_bf2_triage.ps1"
    ps_args = [
        "-GamePath",
        game_path,
        "-TitleId",
        title_id.upper(),
        "-TelemetryDir",
        telemetry_dir,
        "-Configuration",
        ps_config,
    ]
    if fresh:
        ps_args.append("-FreshSession")

    proc = _run_ps1(script, ps_args, repo)
    if proc.returncode != 0:
        return proc.returncode, {
            "error": "launch_bf2_triage.ps1 failed",
            "stdout": proc.stdout,
            "stderr": proc.stderr,
        }

    pid = None
    xenia_exe = None
    for line in (proc.stdout or "").splitlines():
        line = line.strip()
        if line.startswith("PHOENIX_LAUNCH_PID="):
            pid = int(line.split("=", 1)[1])
        if line.startswith("Exe:"):
            xenia_exe = line.split(":", 1)[1].strip()

    debug_port = os.environ.get("PHOENIX_DEBUG_PORT")

    session = None
    if capture_pid:
        rel_exe = xenia_exe
        if xenia_exe and Path(xenia_exe).is_file():
            try:
                rel_exe = str(Path(xenia_exe).resolve().relative_to(repo))
            except ValueError:
                rel_exe = xenia_exe
        session = start_session_record(
            paths,
            pid=pid or 0,
            mode="triage",
            game_path=game_path,
            xenia_exe=rel_exe or _default_xenia_exe("Debug"),
        )

    guard = verify_instrumented_launch(
        paths,
        port=int(debug_port) if debug_port else None,
        wait_sec=18.0,
    )

    exit_code = 0 if guard.get("ok") else 2
    return exit_code, {
        "exit_code": exit_code,
        "mode": "triage",
        "pid": pid,
        "session": session,
        "paths": paths.as_dict(repo),
        "game_basename": Path(game_path).name,
        "phoenix_debug_port": debug_port,
        "launch_guard": guard,
    }


def triage(title_id: str, telemetry_dir: str = "telemetry") -> tuple[int, str]:
    repo = find_repo_root()
    script = repo / "tools" / "tier0" / "triage_gameplay_capture.ps1"
    proc = _run_ps1(
        script,
        ["-TitleId", title_id.upper(), "-TelemetryDir", telemetry_dir],
        repo,
    )
    paths = title_paths(repo, title_id, telemetry_dir)
    report = ""
    if paths.triage_report.is_file():
        report = paths.triage_report.read_text(encoding="utf-8-sig", errors="replace")
    elif proc.stdout:
        report = proc.stdout
    return proc.returncode, report
