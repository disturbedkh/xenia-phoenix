"""phoenixctl command-line interface."""

from __future__ import annotations

import argparse
import json
import os
import sys

from phoenixctl import gates, launch, library_cmd, log_obs, log_scan, probe, repro, session, telemetry, trace_tools
from phoenixctl.launch_guard import check_instrumented_launch
from phoenixctl.paths import find_repo_root, title_paths


def _ensure_utf8_stdout() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        try:
            sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        except (OSError, ValueError):
            pass


def _emit(obj: object, as_json: bool) -> int:
    if as_json:
        print(json.dumps(obj, indent=2))
    else:
        if isinstance(obj, dict):
            print(json.dumps(obj, indent=2))
        else:
            print(obj)
    return 0


def _argv_without_json(argv: list[str]) -> tuple[list[str], bool]:
    """Allow --json before or after subcommands (e.g. repro capture ... --json)."""
    out: list[str] = []
    as_json = False
    for arg in argv:
        if arg == "--json":
            as_json = True
        else:
            out.append(arg)
    return out, as_json


def main(argv: list[str] | None = None) -> int:
    raw_argv = list(argv) if argv is not None else sys.argv[1:]
    raw_argv, json_from_anywhere = _argv_without_json(raw_argv)

    parser = argparse.ArgumentParser(prog="phoenixctl", description="Phoenix Xenia telemetry CLI")
    parser.add_argument("--json", action="store_true", help="JSON output on stdout")
    sub = parser.add_subparsers(dest="command", required=True)

    p_pre = sub.add_parser("preflight", help="Run smoke_session_ready.ps1")
    p_pre.add_argument("--config", default="Release")

    p_launch = sub.add_parser("launch", help="Launch instrumented Xenia")
    launch_sub = p_launch.add_subparsers(dest="launch_mode", required=True)
    p_smoke = launch_sub.add_parser("smoke", help="Timed smoke capture")
    p_smoke.add_argument("--title-id", required=True)
    p_smoke.add_argument("--game", default=None)
    p_smoke.add_argument("--duration", type=int, default=300)
    p_smoke.add_argument("--config", default="Release")
    p_smoke.add_argument("--telemetry-dir", default="telemetry")
    p_triage = launch_sub.add_parser("triage", help="Instrumented triage launch")
    p_triage.add_argument("--title-id", required=True)
    p_triage.add_argument("--game", default=None)
    p_triage.add_argument("--fresh", action="store_true")
    p_triage.add_argument("--duration", type=int, default=0)
    p_triage.add_argument("--config", default="Auto")
    p_triage.add_argument("--telemetry-dir", default="telemetry")

    p_tri = sub.add_parser("triage", help="Post-capture triage report")
    p_tri.add_argument("--title-id", required=True)
    p_tri.add_argument("--telemetry-dir", default="telemetry")

    p_guard = sub.add_parser("guard", help="Verify instrumented launch (CONFIG DUMP)")
    guard_sub = p_guard.add_subparsers(dest="guard_cmd", required=True)
    p_gcheck = guard_sub.add_parser("check", help="Check crash log instrumentation")
    p_gcheck.add_argument("--title-id", required=True)
    p_gcheck.add_argument("--telemetry-dir", default="telemetry")
    p_gcheck.add_argument("--port", type=int, default=None)

    p_tail = sub.add_parser("tail", help="Last N lines of telemetry")
    p_tail.add_argument("kind", choices=telemetry.TAIL_KINDS)
    p_tail.add_argument("--title-id", required=True)
    p_tail.add_argument("--lines", type=int, default=20)
    p_tail.add_argument("--telemetry-dir", default="telemetry")

    p_paths = sub.add_parser("paths", help="Telemetry paths for a title")
    p_paths.add_argument("--title-id", required=True)
    p_paths.add_argument("--telemetry-dir", default="telemetry")

    p_sess = sub.add_parser("session", help="Session PID file")
    sess_sub = p_sess.add_subparsers(dest="session_cmd", required=True)
    sess_sub.add_parser("status")
    p_kill = sess_sub.add_parser("kill")

    p_pat = sub.add_parser("patches", help="list_smoke_patches.py")
    p_pat.add_argument("--title-id", required=True)

    p_agg = sub.add_parser("aggregate-stubs", help="aggregate_stub_hits.py")
    p_agg.add_argument("--title-id", required=True)
    p_agg.add_argument("--telemetry-dir", default="telemetry")

    p_pcm = sub.add_parser("compare-pcm", help="compare_pcm_hash_log.py")
    p_pcm.add_argument("--title-id", required=True)
    p_pcm.add_argument("--baseline", default=None)
    p_pcm.add_argument("--telemetry-dir", default="telemetry")

    p_gate = sub.add_parser("gate", help="Tier 0 offline gates")
    gate_sub = p_gate.add_subparsers(dest="gate_name", required=True)
    g_vmx = gate_sub.add_parser("vmx128")
    g_vmx.add_argument("--iters", type=int, default=10000)
    g_vmx.add_argument("--seed", type=int, default=42)
    g_vmx.add_argument("--config", default="Release")
    gate_sub.add_parser("gpu-replay").add_argument("--config", default="Release")
    gate_sub.add_parser("patch-debt")

    p_probe = sub.add_parser("probe", help="localhost phoenix_probe HTTP")
    probe_sub = p_probe.add_subparsers(dest="probe_cmd", required=True)
    p_ps = probe_sub.add_parser("status")
    p_ps.add_argument("--port", type=int, default=None)
    p_ph = probe_sub.add_parser("health")
    p_ph.add_argument("--port", type=int, default=None)
    p_pc = probe_sub.add_parser("cvars")
    p_pc.add_argument("--names", default="kernel_stub_hit_log,apu_pcm_hash_log")
    p_pc.add_argument("--port", type=int, default=None)
    p_psnap = probe_sub.add_parser("snapshot", help="health+status+snapshot+cvars")
    p_psnap.add_argument("--port", type=int, default=None)
    p_pev = probe_sub.add_parser("events")
    p_pev.add_argument("--since-seq", type=int, default=0)
    p_pev.add_argument("--port", type=int, default=None)

    p_log = sub.add_parser("log", help="Crash log analysis")
    log_sub = p_log.add_subparsers(dest="log_cmd", required=True)
    p_logscan = log_sub.add_parser("scan")
    p_logscan.add_argument("--title-id", required=True)
    p_logscan.add_argument("--telemetry-dir", default="telemetry")

    p_logsum = log_sub.add_parser("summarize", help="Summarize obs JSONL events")
    p_logsum.add_argument("--title-id", required=True)
    p_logsum.add_argument("--telemetry-dir", default="telemetry")
    p_logsum.add_argument("--write-summary", action="store_true",
                          help="Write telemetry/{tid}_obs_summary.json")
    p_logsum.add_argument("--visual-pass", action="store_true")
    p_logsum.add_argument("--visual-fail", action="store_true")

    p_repro = sub.add_parser("repro", help="GPU repro proof-loop handoff")
    repro_sub = p_repro.add_subparsers(dest="repro_cmd", required=True)
    p_rcap = repro_sub.add_parser("capture", help="gpu_repro_capture.ps1")
    p_rcap.add_argument("--title-id", required=True)
    p_rcap.add_argument("--game", default=None)
    p_rcap.add_argument("--config", default="Debug")
    p_rcap.add_argument("--telemetry-dir", default="telemetry")
    p_rcap.add_argument("--launch", action="store_true")
    p_rcap.add_argument("--post-only", action="store_true")
    p_rcap.add_argument("--gfx-workaround", action="store_true")
    p_rcap.add_argument("--release-menu", action="store_true", help="Use Release build (smoother menu)")
    p_rcap.add_argument("--poll-sec", type=int, default=5)
    p_rcap.add_argument("--poll-count", type=int, default=6)
    p_rcap.add_argument("--trace-path", default=None)
    p_rcap.add_argument("--run-replay", action="store_true")

    p_trace = sub.add_parser("trace", help="GPU .xtr tools")
    trace_sub = p_trace.add_subparsers(dest="trace_cmd", required=True)
    p_trv = trace_sub.add_parser("validate")
    p_trv.add_argument("path")
    p_trd = trace_sub.add_parser("dump")
    p_trd.add_argument("path")
    p_trd.add_argument("--config", default="Release")
    p_tre = trace_sub.add_parser("explain", help="Join .xtr.obs.json to events JSONL")
    p_tre.add_argument("path")
    p_tre.add_argument("--title-id", default=None)
    p_tre.add_argument("--telemetry-dir", default="telemetry")

    p_obs = sub.add_parser("obs", help="Live observability JSONL")
    obs_sub = p_obs.add_subparsers(dest="obs_cmd", required=True)
    p_obst = obs_sub.add_parser("tail", help="Tail events.jsonl")
    p_obst.add_argument("--title-id", required=True)
    p_obst.add_argument("--lines", type=int, default=20)
    p_obst.add_argument("--code", default=None)
    p_obst.add_argument("--channel", default=None)
    p_obst.add_argument("--telemetry-dir", default="telemetry")

    library_cmd.register_library_subparser(sub)

    args = parser.parse_args(raw_argv)
    as_json = args.json or json_from_anywhere
    repo = find_repo_root()

    try:
        if args.command == "preflight":
            code, out = launch.preflight(args.config)
            if not as_json:
                print(out)
            return code if not as_json else _emit({"exit_code": code, "output": out}, True) or code

        if args.command == "launch":
            if args.launch_mode == "smoke":
                code, result = launch.launch_smoke(
                    args.title_id,
                    args.game,
                    duration=args.duration,
                    config=args.config,
                    telemetry_dir=args.telemetry_dir,
                )
            else:
                code, result = launch.launch_triage(
                    args.title_id,
                    args.game,
                    fresh=args.fresh,
                    duration=args.duration,
                    config=args.config,
                    telemetry_dir=args.telemetry_dir,
                )
            if as_json:
                result["exit_code"] = code
                print(json.dumps(result, indent=2))
            else:
                print(json.dumps(result, indent=2))
                guard = result.get("launch_guard")
                if guard and not guard.get("ok"):
                    print("WARN: launch_guard failed — fix launch before play:", file=sys.stderr)
                    for issue in guard.get("issues", []):
                        print(f"  - {issue}", file=sys.stderr)
            return code

        if args.command == "guard":
            if args.guard_cmd != "check":
                parser.error("unknown guard subcommand")
            paths = title_paths(repo, args.title_id, args.telemetry_dir)
            port = args.port
            if port is None and os.environ.get("PHOENIX_DEBUG_PORT"):
                try:
                    port = int(os.environ["PHOENIX_DEBUG_PORT"])
                except ValueError:
                    port = None
            data = check_instrumented_launch(paths, port=port)
            data["exit_code"] = 0 if data["ok"] else 2
            if as_json:
                print(json.dumps(data, indent=2))
            else:
                print(json.dumps(data, indent=2))
                if not data["ok"]:
                    for issue in data["issues"]:
                        print(f"  - {issue}", file=sys.stderr)
            return data["exit_code"]

        if args.command == "triage":
            _ensure_utf8_stdout()
            code, report = launch.triage(args.title_id, args.telemetry_dir)
            if as_json:
                print(json.dumps({"exit_code": code, "report": report}, indent=2))
            else:
                print(report)
            return code

        if args.command == "tail":
            data = telemetry.tail_lines(
                args.title_id, args.kind, args.lines, args.telemetry_dir
            )
            if as_json:
                print(json.dumps(data, indent=2))
            else:
                for line in data.get("lines", []):
                    print(line)
            return 0

        if args.command == "paths":
            paths = title_paths(repo, args.title_id, args.telemetry_dir)
            print(json.dumps(paths.as_dict(repo), indent=2))
            return 0

        if args.command == "session":
            tid = session.read_session_title_from_repo(repo) or "00000000"
            tp = title_paths(repo, tid)
            if args.session_cmd == "status":
                data = session.session_status(tp)
                print(json.dumps(data, indent=2))
                return 0
            data = session.kill_session(tp)
            print(json.dumps(data, indent=2))
            return 0

        if args.command == "patches":
            code, out = telemetry.list_patches(args.title_id)
            if as_json:
                print(json.dumps({"exit_code": code, "output": out}, indent=2))
            else:
                print(out)
            return code

        if args.command == "aggregate-stubs":
            code, data = telemetry.aggregate_stubs(args.title_id, args.telemetry_dir)
            if as_json:
                print(json.dumps({"exit_code": code, "data": data}, indent=2))
            else:
                print(json.dumps(data, indent=2))
            return code

        if args.command == "compare-pcm":
            code, out = telemetry.compare_pcm(
                args.title_id, args.baseline, args.telemetry_dir
            )
            if as_json:
                print(json.dumps({"exit_code": code, "output": out}, indent=2))
            else:
                print(out)
            return code

        if args.command == "gate":
            if args.gate_name == "vmx128":
                code, out = gates.gate_vmx128(args.iters, args.seed, args.config)
            elif args.gate_name == "gpu-replay":
                code, out = gates.gate_gpu_replay(args.config)
            else:
                code, out = gates.gate_patch_debt()
            if as_json:
                print(json.dumps({"exit_code": code, "output": out}, indent=2))
            else:
                print(out)
            return code

        if args.command == "probe":
            port = args.port
            if args.probe_cmd == "status":
                code, data = probe.probe_status(port)
                print(json.dumps({"http_status": code, "body": data}, indent=2))
                return 0 if code == 200 else 1
            if args.probe_cmd == "health":
                code, data = probe.probe_health(port)
                print(json.dumps({"http_status": code, "body": data}, indent=2))
                return 0 if code == 200 else 1
            if args.probe_cmd == "snapshot":
                data = probe.probe_snapshot_full(port)
                print(json.dumps(data, indent=2))
                return 0 if data.get("ok") else 1
            if args.probe_cmd == "events":
                code, data = probe.probe_events(args.since_seq, port)
                print(json.dumps({"http_status": code, "body": data}, indent=2))
                return 0 if code == 200 else 1
            code, data = probe.probe_cvars(args.names, port)
            print(json.dumps({"http_status": code, "body": data}, indent=2))
            return 0 if code == 200 else 1

        if args.command == "log":
            if args.log_cmd == "summarize":
                snap = probe.probe_snapshot_full()
                probe_body = snap.get("snapshot", {}).get("body") if snap.get("ok") else None
                visual = None
                if args.visual_pass:
                    visual = True
                elif args.visual_fail:
                    visual = False
                guard_ok = None
                try:
                    paths = title_paths(repo, args.title_id, args.telemetry_dir)
                    guard = check_instrumented_launch(paths)
                    guard_ok = guard.get("ok")
                except (RuntimeError, OSError, ValueError):
                    guard_ok = None
                data = log_obs.summarize(
                    repo,
                    args.title_id,
                    args.telemetry_dir,
                    probe_body,
                    launch_guard_ok=guard_ok,
                    visual_pass=visual,
                )
                if args.write_summary:
                    log_obs.write_obs_summary_json(
                        repo,
                        args.title_id,
                        args.telemetry_dir,
                        probe_snapshot=probe_body,
                        launch_guard_ok=guard_ok,
                        visual_pass=visual,
                    )
                print(json.dumps(data, indent=2))
                return 0
            data = log_scan.scan_crash_log(args.title_id, args.telemetry_dir)
            print(json.dumps(data, indent=2))
            return 0

        if args.command == "repro":
            code, data = repro.repro_capture(
                args.title_id,
                game=args.game,
                config=args.config,
                telemetry_dir=args.telemetry_dir,
                launch=args.launch,
                post_only=args.post_only,
                gfx_workaround=args.gfx_workaround,
                release_menu=args.release_menu,
                poll_sec=args.poll_sec,
                poll_count=args.poll_count,
                trace_path=args.trace_path,
                run_replay=args.run_replay,
            )
            print(json.dumps(data, indent=2))
            return code

        if args.command == "library":
            return args.func(args)

        if args.command == "trace":
            from pathlib import Path

            trace_path = Path(args.path)
            if args.trace_cmd == "validate":
                code, out = trace_tools.validate_trace(trace_path)
                print(json.dumps({"exit_code": code, "output": out}, indent=2))
                return code
            if args.trace_cmd == "explain":
                data = trace_tools.explain_trace(
                    trace_path,
                    title_id=args.title_id,
                    telemetry_dir=args.telemetry_dir,
                )
                print(json.dumps(data, indent=2))
                return 0
            code, out = trace_tools.dump_trace(trace_path, args.config)
            print(json.dumps({"exit_code": code, "output": out}, indent=2))
            return code

        if args.command == "obs":
            if args.obs_cmd == "tail":
                events = log_obs.tail_events(
                    repo,
                    args.title_id,
                    args.telemetry_dir,
                    lines=args.lines,
                    code_filter=args.code,
                    channel_filter=args.channel,
                )
                print(json.dumps({"events": events}, indent=2))
                return 0

    except (RuntimeError, ValueError) as e:
        if as_json:
            print(json.dumps({"error": str(e)}, indent=2))
        else:
            print(f"error: {e}", file=sys.stderr)
        return 1

    return 1

