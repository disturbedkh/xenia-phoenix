"""stdio MCP server — Phoenix telemetry and control tools."""

from __future__ import annotations

import json
import os
from typing import Any

from mcp.server.fastmcp import FastMCP

from phoenixctl import gates, launch, probe, session, telemetry
from phoenixctl.paths import find_repo_root, title_paths

# Ensure repo root is set when Cursor launches MCP with PHOENIX_SRC.
if os.environ.get("PHOENIX_SRC"):
    os.chdir(os.environ["PHOENIX_SRC"])

mcp = FastMCP("phoenix")


def _json(obj: Any) -> str:
    return json.dumps(obj, indent=2)


@mcp.tool()
def phoenix_preflight(config: str = "Release") -> str:
    """Run smoke session preflight (build artifacts + scripts)."""
    code, out = launch.preflight(config)
    return _json({"exit_code": code, "output": out})


@mcp.tool()
def phoenix_launch_smoke(
    title_id: str,
    game_path: str = "",
    duration_sec: int = 300,
    config: str = "Release",
) -> str:
    """Timed smoke capture with stub/XMA/PCM telemetry."""
    game = game_path or None
    code, result = launch.launch_smoke(
        title_id, game, duration=duration_sec, config=config
    )
    result["exit_code"] = code
    return _json(result)


@mcp.tool()
def phoenix_launch_triage(
    title_id: str,
    game_path: str = "",
    fresh_session: bool = False,
    duration_sec: int = 0,
) -> str:
    """Launch instrumented Xenia for manual play-to-crash triage."""
    game = game_path or None
    code, result = launch.launch_triage(
        title_id, game, fresh=fresh_session, duration=duration_sec
    )
    result["exit_code"] = code
    return _json(result)


@mcp.tool()
def phoenix_triage(title_id: str) -> str:
    """Run post-capture triage gates; returns report text and exit code."""
    code, report = launch.triage(title_id)
    return _json({"exit_code": code, "report": report})


@mcp.tool()
def phoenix_tail(
    title_id: str,
    kind: str = "stubs",
    lines: int = 20,
) -> str:
    """Last N lines of stubs, xma, pcm, or crash telemetry."""
    if kind not in telemetry.TAIL_KINDS:
        return _json({"error": f"kind must be one of {telemetry.TAIL_KINDS}"})
    return _json(telemetry.tail_lines(title_id, kind, lines))


@mcp.tool()
def phoenix_paths(title_id: str) -> str:
    """Telemetry file paths for a title (relative to xenia-phoenix-src)."""
    repo = find_repo_root()
    return _json(title_paths(repo, title_id).as_dict(repo))


@mcp.tool()
def phoenix_session_status() -> str:
    """Active phoenixctl session (PID, title, log paths)."""
    repo = find_repo_root()
    tid = session.read_session_title_from_repo(repo) or "00000000"
    return _json(session.session_status(title_paths(repo, tid)))


@mcp.tool()
def phoenix_session_kill() -> str:
    """Stop Xenia process started by phoenixctl launch triage."""
    repo = find_repo_root()
    tid = session.read_session_title_from_repo(repo) or "00000000"
    return _json(session.kill_session(title_paths(repo, tid)))


@mcp.tool()
def phoenix_list_patches(title_id: str) -> str:
    """List smoke patches for a title ID."""
    code, out = telemetry.list_patches(title_id)
    return _json({"exit_code": code, "output": out})


@mcp.tool()
def phoenix_aggregate_stubs(title_id: str) -> str:
    """Aggregate kernel stub JSONL for a title."""
    code, data = telemetry.aggregate_stubs(title_id)
    return _json({"exit_code": code, "data": data})


@mcp.tool()
def phoenix_compare_pcm(title_id: str, baseline_path: str = "") -> str:
    """Compare PCM hash JSONL vs baseline."""
    base = baseline_path or None
    code, out = telemetry.compare_pcm(title_id, base)
    return _json({"exit_code": code, "output": out})


@mcp.tool()
def phoenix_gate_vmx128(iterations: int = 10000, seed: int = 42) -> str:
    """Run vmx128-fuzz Tier 0 gate."""
    code, out = gates.gate_vmx128(iterations, seed)
    return _json({"exit_code": code, "output": out})


@mcp.tool()
def phoenix_gate_gpu_replay() -> str:
    """Run GPU trace replay CI (slow; requires built xenia + traces)."""
    code, out = gates.gate_gpu_replay()
    return _json({"exit_code": code, "output": out})


@mcp.tool()
def phoenix_probe_status(port: int = 0) -> str:
    """GET /status from localhost phoenix_probe (set PHOENIX_DEBUG_PORT on launch)."""
    p = port if port > 0 else None
    code, body = probe.probe_status(p)
    return _json({"http_status": code, "body": body})


@mcp.tool()
def phoenix_probe_health(port: int = 0) -> str:
    """GET /health from localhost phoenix_probe."""
    p = port if port > 0 else None
    code, body = probe.probe_health(p)
    return _json({"http_status": code, "body": body})


@mcp.tool()
def phoenix_probe_cvars(names: str = "", port: int = 0) -> str:
    """GET /cvars?names= from localhost phoenix_probe."""
    p = port if port > 0 else None
    n = names or probe.SNAPSHOT_CVAR_NAMES
    code, body = probe.probe_cvars(n, p)
    return _json({"http_status": code, "body": body})


@mcp.tool()
def phoenix_probe_snapshot(port: int = 0) -> str:
    """Unified live probe: health, status, snapshot, key cvars."""
    p = port if port > 0 else None
    return _json(probe.probe_snapshot_full(p))


@mcp.tool()
def phoenix_probe_events(since_seq: int = 0, port: int = 0) -> str:
    """GET /events?since_seq= from localhost phoenix_probe."""
    p = port if port > 0 else None
    code, body = probe.probe_events(since_seq, p)
    return _json({"http_status": code, "body": body})


@mcp.tool()
def phoenix_log_scan(title_id: str) -> str:
    """Scan crash log for GPU/APU buckets and upload-range error count."""
    from phoenixctl import log_scan as log_scan_mod

    return _json(log_scan_mod.scan_crash_log(title_id))


@mcp.tool()
def phoenix_log_summarize(
    title_id: str,
    telemetry_dir: str = "telemetry",
    visual_pass: bool = False,
    visual_fail: bool = False,
) -> str:
    """Summarize obs events JSONL + BF2 classification."""
    from pathlib import Path

    from phoenixctl import log_obs as log_obs_mod
    from phoenixctl import probe as probe_mod
    from phoenixctl.launch_guard import check_instrumented_launch
    from phoenixctl.paths import find_repo_root, title_paths

    repo = find_repo_root()
    snap = probe_mod.probe_snapshot_full()
    probe_body = snap.get("snapshot", {}).get("body") if snap.get("ok") else None
    paths = title_paths(repo, title_id, telemetry_dir)
    guard = check_instrumented_launch(paths)
    visual = True if visual_pass else (False if visual_fail else None)
    data = log_obs_mod.summarize(
        Path(repo),
        title_id,
        telemetry_dir,
        probe_body,
        launch_guard_ok=guard.get("ok"),
        visual_pass=visual,
    )
    return _json(data)


@mcp.tool()
def phoenix_obs_tail(
    title_id: str,
    lines: int = 20,
    code: str = "",
    channel: str = "",
    telemetry_dir: str = "telemetry",
) -> str:
    """Tail structured obs events JSONL for a title."""
    from pathlib import Path

    from phoenixctl import log_obs as log_obs_mod
    from phoenixctl.paths import find_repo_root

    repo = find_repo_root()
    events = log_obs_mod.tail_events(
        Path(repo),
        title_id,
        telemetry_dir,
        lines=lines,
        code_filter=code or None,
        channel_filter=channel or None,
    )
    return _json({"events": events})


@mcp.tool()
def phoenix_trace_validate(trace_path: str) -> str:
    """Validate a .xtr GPU trace file."""
    from pathlib import Path

    from phoenixctl import trace_tools as trace_tools_mod

    code, out = trace_tools_mod.validate_trace(Path(trace_path))
    return _json({"exit_code": code, "output": out})


@mcp.tool()
def phoenix_repro_capture(
    title_id: str,
    game_path: str = "",
    launch: bool = False,
    post_only: bool = True,
    gfx_workaround: bool = False,
    release_menu: bool = False,
    poll_count: int = 6,
    poll_sec: int = 5,
    trace_path: str = "",
    run_replay: bool = False,
) -> str:
    """GPU proof-loop: probe poll, log scan, trace validate, fix-bar hints."""
    from phoenixctl import repro as repro_mod

    code, data = repro_mod.repro_capture(
        title_id,
        game=game_path or None,
        launch=launch,
        post_only=post_only,
        gfx_workaround=gfx_workaround,
        release_menu=release_menu,
        poll_sec=poll_sec,
        poll_count=poll_count,
        trace_path=trace_path or None,
        run_replay=run_replay,
    )
    data["exit_code"] = code
    return _json(data)


def main() -> None:
    mcp.run()


if __name__ == "__main__":
    main()
