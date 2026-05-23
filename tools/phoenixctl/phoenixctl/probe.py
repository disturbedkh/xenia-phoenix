"""HTTP client for in-emulator phoenix_probe (localhost)."""

from __future__ import annotations

import json
import os
import urllib.error
import urllib.request
from typing import Any


def default_port() -> int:
    raw = os.environ.get("PHOENIX_DEBUG_PORT", "8765")
    try:
        return int(raw)
    except ValueError:
        return 8765


def probe_get(path: str, port: int | None = None, timeout: float = 2.0) -> tuple[int, Any]:
    p = port if port is not None else default_port()
    url = f"http://127.0.0.1:{p}{path}"
    try:
        with urllib.request.urlopen(url, timeout=timeout) as resp:
            body = resp.read().decode("utf-8", errors="replace")
            return resp.status, json.loads(body) if body.strip() else {}
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", errors="replace")
        try:
            return e.code, json.loads(body)
        except json.JSONDecodeError:
            return e.code, {"error": body}
    except urllib.error.URLError as e:
        return 0, {"error": str(e.reason), "url": url}
    except json.JSONDecodeError as e:
        return 0, {"error": f"invalid json: {e}"}


def probe_status(port: int | None = None) -> tuple[int, Any]:
    return probe_get("/status", port=port)


def probe_health(port: int | None = None) -> tuple[int, Any]:
    return probe_get("/health", port=port)


def probe_cvars(names: str, port: int | None = None) -> tuple[int, Any]:
    from urllib.parse import quote

    q = quote(names, safe=",")
    return probe_get(f"/cvars?names={q}", port=port)


def probe_snapshot(port: int | None = None) -> tuple[int, Any]:
    return probe_get("/snapshot", port=port)


def probe_events(since_seq: int = 0, port: int | None = None) -> tuple[int, Any]:
    return probe_get(f"/events?since_seq={since_seq}", port=port)


SNAPSHOT_CVAR_NAMES = (
    "readback_resolve,hid,phoenix_debug_port,log_preset,obs_events_log,"
    "kernel_stub_hit_log,"
    "apu_pcm_hash_log,trace_gpu_prefix"
)


def probe_snapshot_full(port: int | None = None) -> dict[str, Any]:
    """Health + status + snapshot + key cvars in one dict for agents."""
    p = port if port is not None else default_port()
    health_code, health = probe_health(p)
    status_code, status = probe_status(p)
    snap_code, snapshot = probe_snapshot(p)
    cvar_code, cvars_body = probe_cvars(SNAPSHOT_CVAR_NAMES, p)
    return {
        "port": p,
        "health": {"http_status": health_code, "body": health},
        "status": {"http_status": status_code, "body": status},
        "snapshot": {"http_status": snap_code, "body": snapshot},
        "cvars": {"http_status": cvar_code, "body": cvars_body},
        "ok": health_code == 200 and snap_code == 200,
    }
