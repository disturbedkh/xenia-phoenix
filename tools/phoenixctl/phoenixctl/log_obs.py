"""Unified observability JSONL summarization and BF2 classification."""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path
from typing import Any

from phoenixctl.obs_schema import load_schema, validate_event


BF2_TITLE = "454107DB"

INVARIANT_TO_FINDING: dict[str, tuple[str, str | None]] = {
    "DepthHostSidecarStale": ("U-GPU-001", "G-454107DB-003"),
    "HostDepthTransferMismatch": ("U-GPU-001", "G-454107DB-003"),
    "HostDepthStore": ("U-GPU-001", "G-454107DB-003"),
    "GpuUploadRangeError": ("U-GPU-001", None),
    "XmaDivergence": (None, None),
}


def _read_jsonl(
    path: Path, schema: dict[str, Any] | None = None
) -> tuple[list[dict[str, Any]], int]:
    events: list[dict[str, Any]] = []
    invalid = 0
    if not path.is_file():
        return events, invalid
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            obj = json.loads(line)
        except json.JSONDecodeError:
            invalid += 1
            continue
        if schema and not validate_event(obj, schema):
            invalid += 1
            continue
        events.append(obj)
    return events, invalid


def _events_path(repo: Path, title_id: str, telemetry_dir: str) -> Path:
    tid = title_id.lower().replace("0x", "")
    return repo / telemetry_dir / f"{tid}_events.jsonl"


def _xtr_files(repo: Path, title_id: str, telemetry_dir: str) -> list[str]:
    tid = title_id.lower().replace("0x", "")
    trace_dir = repo / telemetry_dir / f"{tid}_gpu_trace"
    if not trace_dir.is_dir():
        return []
    return sorted(
        str(p.relative_to(repo)).replace("\\", "/")
        for p in trace_dir.glob("*.xtr")
    )


def _probe_gpu(probe_snapshot: dict | None) -> dict[str, Any] | None:
    if not probe_snapshot:
        return None
    if isinstance(probe_snapshot.get("gpu"), dict):
        return probe_snapshot["gpu"]
    body = probe_snapshot.get("body")
    if isinstance(body, dict) and isinstance(body.get("gpu"), dict):
        return body["gpu"]
    snap = probe_snapshot.get("snapshot")
    if isinstance(snap, dict) and isinstance(snap.get("body"), dict):
        b = snap["body"]
        if isinstance(b.get("gpu"), dict):
            return b["gpu"]
    return None


def classify_bf2(events: list[dict[str, Any]], probe_snapshot: dict | None) -> dict[str, Any]:
    code_counts: Counter[str] = Counter()
    domain_counts: Counter[str] = Counter()
    for ev in events:
        code = str(ev.get("code", ""))
        if code:
            code_counts[code] += 1
        domain = str(ev.get("domain", ""))
        if domain:
            domain_counts[domain] += 1

    dominant = code_counts.most_common(5)
    classification: dict[str, Any] = {
        "title_id": BF2_TITLE,
        "symptom_bucket": None,
        "finding_ids": [],
        "dominant_codes": [{"code": c, "count": n} for c, n in dominant],
        "domain_counts": dict(domain_counts),
    }

    def add_finding(universal: str | None, game: str | None) -> None:
        if universal and universal not in classification["finding_ids"]:
            classification["finding_ids"].append(universal)
        if game and game not in classification["finding_ids"]:
            classification["finding_ids"].append(game)
        classification["symptom_bucket"] = classification["symptom_bucket"] or "graphics"

    for code, _ in dominant:
        if code in INVARIANT_TO_FINDING:
            add_finding(*INVARIANT_TO_FINDING[code])
            break

    gpu = _probe_gpu(probe_snapshot)
    if isinstance(gpu, dict):
        classification["probe_gpu"] = gpu
        probe_signals = (
            gpu.get("host_depth_transfer_mismatch_count", 0) > 0
            or gpu.get("obs_host_depth_transfer_mismatch_count", 0) > 0
            or gpu.get("obs_depth_host_sidecar_stale_count", 0) > 0
            or gpu.get("upload_range_error_count", 0) > 0
            or gpu.get("obs_gpu_upload_range_error_count", 0) > 0
        )
        if probe_signals:
            add_finding("U-GPU-001", "G-454107DB-003")

    if not classification["symptom_bucket"] and not code_counts:
        classification["symptom_bucket"] = "unknown"
    elif not classification["symptom_bucket"]:
        classification["symptom_bucket"] = "telemetry_only"

    return classification


def summarize(
    repo: Path,
    title_id: str,
    telemetry_dir: str = "telemetry",
    probe_snapshot: dict | None = None,
    *,
    launch_guard_ok: bool | None = None,
    visual_pass: bool | None = None,
) -> dict[str, Any]:
    events_path = _events_path(repo, title_id, telemetry_dir)
    schema = load_schema(repo)
    events, invalid_count = _read_jsonl(events_path, schema)

    if not events_path.is_file():
        events_status = "no_events_file"
    elif not events:
        events_status = "empty"
    else:
        events_status = "ok"

    ts_values = [int(e["ts_ms"]) for e in events if "ts_ms" in e]
    summary: dict[str, Any] = {
        "title_id": title_id.upper().replace("0x", ""),
        "events_path": str(events_path.relative_to(repo)).replace("\\", "/")
        if events_path.is_file()
        else str(events_path),
        "events_status": events_status,
        "event_count": len(events),
        "invalid_event_count": invalid_count,
        "time_range_ms": {
            "min": min(ts_values) if ts_values else None,
            "max": max(ts_values) if ts_values else None,
        },
        "top_codes": [],
        "domains": {},
        "xtr_files": _xtr_files(repo, title_id, telemetry_dir),
        "gates": {
            "launch_guard": launch_guard_ok,
            "visual_verify": visual_pass,
        },
    }

    code_counts: Counter[str] = Counter()
    domain_counts: Counter[str] = Counter()
    for ev in events:
        code_counts[str(ev.get("code", ""))] += 1
        domain_counts[str(ev.get("domain", ""))] += 1
    summary["top_codes"] = [
        {"code": c, "count": n} for c, n in code_counts.most_common(15) if c
    ]
    summary["domains"] = {k: v for k, v in domain_counts.items() if k}

    tid = title_id.upper().replace("0x", "")
    if tid == BF2_TITLE:
        summary["classification"] = classify_bf2(events, probe_snapshot)

    return summary


def write_obs_summary_json(
    repo: Path,
    title_id: str,
    telemetry_dir: str = "telemetry",
    **kwargs: Any,
) -> Path:
    data = summarize(repo, title_id, telemetry_dir, **kwargs)
    tid = title_id.lower().replace("0x", "")
    out = repo / telemetry_dir / f"{tid}_obs_summary.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(data, indent=2), encoding="utf-8")
    return out


def tail_events(
    repo: Path,
    title_id: str,
    telemetry_dir: str = "telemetry",
    *,
    lines: int = 20,
    code_filter: str | None = None,
    channel_filter: str | None = None,
) -> list[dict[str, Any]]:
    path = _events_path(repo, title_id, telemetry_dir)
    if not path.is_file():
        return []
    raw = [
        ln.strip()
        for ln in path.read_text(encoding="utf-8", errors="replace").splitlines()
        if ln.strip()
    ]
    out: list[dict[str, Any]] = []
    for line in raw[-lines:]:
        try:
            obj = json.loads(line)
        except json.JSONDecodeError:
            continue
        if code_filter and obj.get("code") != code_filter:
            continue
        if channel_filter and obj.get("channel") != channel_filter:
            continue
        out.append(obj)
    return out
