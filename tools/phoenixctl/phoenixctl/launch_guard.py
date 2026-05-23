"""Verify instrumented launch configuration (crash log + optional probe)."""

from __future__ import annotations

import re
import time
from typing import Any

from phoenixctl import probe
from phoenixctl.paths import TitlePaths


def _cvar_str(val: object) -> str:
    if val is None:
        return ""
    s = str(val).strip()
    if len(s) >= 2 and s[0] == '"' and s[-1] == '"':
        return s[1:-1]
    return s


def _parse_config_dump(log_text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    section = ""
    for line in log_text.splitlines():
        if line.strip().endswith("]") and line.strip().startswith("["):
            section = line.strip().strip("[]").lower()
            continue
        m = re.match(r"^([a-z0-9_]+)\s*=\s*(.+?)\s*$", line.strip(), re.I)
        if not m:
            continue
        key, val = m.group(1).lower(), m.group(2).strip().strip('"')
        out[key if not section else f"{section}.{key}"] = val
        out[key] = val
    return out


def _artifact_checks(paths: TitlePaths) -> tuple[list[str], dict[str, Any]]:
    """Runtime instrumentation via telemetry files (CONFIG DUMP is TOML-only)."""
    issues: list[str] = []
    evidence: dict[str, Any] = {}

    if not paths.crash.is_file():
        issues.append(f"missing crash log: {paths.crash}")
        return issues, evidence

    crash_text = paths.crash.read_text(encoding="utf-8-sig", errors="replace")
    evidence["crash_log_bytes"] = len(crash_text)
    if "PhoenixProbe:" in crash_text:
        evidence["phoenix_probe_log"] = True
    if "CONFIG DUMP" in crash_text:
        evidence["config_dump_present"] = True

    if paths.stubs.is_file():
        stub_lines = [
            ln
            for ln in paths.stubs.read_text(encoding="utf-8", errors="replace").splitlines()
            if ln.strip()
        ]
        evidence["stub_lines"] = len(stub_lines)
        if not stub_lines:
            issues.append(f"empty stub log: {paths.stubs}")
    else:
        issues.append(f"missing stub log: {paths.stubs}")

    if paths.pcm.is_file():
        pcm_lines = [
            ln
            for ln in paths.pcm.read_text(encoding="utf-8", errors="replace").splitlines()
            if ln.strip()
        ]
        evidence["pcm_lines"] = len(pcm_lines)
        if not pcm_lines:
            issues.append(f"empty pcm log: {paths.pcm}")
    else:
        issues.append(f"missing pcm log: {paths.pcm}")

    gpu_traces = list(paths.gpu_trace.glob("*.xtr")) if paths.gpu_trace.is_dir() else []
    evidence["gpu_trace_count"] = len(gpu_traces)

    if paths.events.is_file():
        ev_lines = [
            ln
            for ln in paths.events.read_text(encoding="utf-8", errors="replace").splitlines()
            if ln.strip()
        ]
        evidence["events_lines"] = len(ev_lines)
    else:
        issues.append(f"missing obs events log: {paths.events}")

    return issues, evidence


def _probe_launch_checks(port: int) -> tuple[list[str], dict[str, Any]]:
    issues: list[str] = []
    probe_data: dict[str, Any] = {}

    code, body = probe.probe_health(port)
    probe_data["health"] = {"http_status": code, "body": body}
    if code != 200:
        issues.append(f"probe /health HTTP {code}: {body}")
        return issues, probe_data

    sc, snap = probe.probe_get("/snapshot", port=port)
    probe_data["snapshot"] = {"http_status": sc, "body": snap}
    if sc != 200 or not isinstance(snap, dict):
        issues.append(f"probe /snapshot HTTP {sc}")
        return issues, probe_data

    launch = snap.get("launch") if isinstance(snap.get("launch"), dict) else {}
    gpu = snap.get("gpu") if isinstance(snap.get("gpu"), dict) else {}

    readback = _cvar_str(gpu.get("readback_resolve"))
    if readback != "fast":
        issues.append(f"readback_resolve={readback!r} (expected fast)")

    hid = _cvar_str(launch.get("hid"))
    if hid and hid != "xinput":
        issues.append(f"hid={hid!r} (expected xinput)")

    stub_log = _cvar_str(launch.get("kernel_stub_hit_log"))
    pcm_log = _cvar_str(launch.get("apu_pcm_hash_log"))
    obs_events = _cvar_str(launch.get("obs_events_log"))
    log_preset = _cvar_str(launch.get("log_preset"))
    if not stub_log:
        issues.append("kernel_stub_hit_log empty (runtime)")
    if not pcm_log:
        issues.append("apu_pcm_hash_log empty (runtime)")
    if not obs_events:
        issues.append("obs_events_log empty (runtime)")
    if log_preset and log_preset != "develop":
        issues.append(f"log_preset={log_preset!r} (expected develop for triage)")

    trace_gpu = _cvar_str(launch.get("trace_gpu_prefix")).replace("\\", "/").lower()
    if trace_gpu and "telemetry" not in trace_gpu:
        issues.append(
            f"trace_gpu_prefix not under telemetry: "
            f"{launch.get('trace_gpu_prefix')!r}"
        )

    return issues, probe_data


def check_instrumented_launch(
    paths: TitlePaths,
    *,
    port: int | None = None,
) -> dict[str, Any]:
    """Verify instrumentation via probe (live) and/or telemetry artifacts."""
    issues: list[str] = []
    config: dict[str, str] = {}
    probe_data: dict[str, Any] = {}
    evidence: dict[str, Any] = {}

    if paths.crash.is_file():
        text = paths.crash.read_text(encoding="utf-8-sig", errors="replace")
        if "CONFIG DUMP" in text:
            chunk = text.split("CONFIG DUMP", 1)[1]
            chunk = chunk.split("END OF CONFIG DUMP", 1)[0]
            config = _parse_config_dump(chunk)
            evidence["config_dump_note"] = (
                "CONFIG DUMP is xenia-canary.config.toml on disk, not runtime CLI."
            )

    art_issues, art_evidence = _artifact_checks(paths)
    evidence.update(art_evidence)
    issues.extend(art_issues)

    if port:
        probe_issues, probe_data = _probe_launch_checks(port)
        health_ok = probe_data.get("health", {}).get("http_status") == 200
        if health_ok:
            issues.extend(probe_issues)
        elif evidence.get("pcm_lines", 0) > 0 and evidence.get("stub_lines", 0) > 0:
            evidence["probe_offline"] = True
        else:
            issues.extend(probe_issues)
    elif evidence.get("pcm_lines", 0) > 0 and evidence.get("stub_lines", 0) > 0:
        # Post-session: probe offline but telemetry proves instrumented launch.
        pass
    elif evidence.get("config_dump_present") and config.get("readback_resolve") == "none":
        issues.append(
            "No pcm/stub telemetry and CONFIG DUMP shows defaults; "
            "use bf2_gpu_session.ps1 -Launch, not raw xenia_canary.exe"
        )

    return {
        "ok": len(issues) == 0,
        "issues": issues,
        "config": config,
        "probe": probe_data,
        "evidence": evidence,
    }


def verify_instrumented_launch(
    paths: TitlePaths,
    *,
    port: int | None = None,
    wait_sec: float = 14.0,
) -> dict[str, Any]:
    """Wait for log/probe then return {ok, issues, config, probe, evidence}."""
    time.sleep(wait_sec)
    return check_instrumented_launch(paths, port=port)
