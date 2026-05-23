"""Unit tests for phoenixctl.log_obs."""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from phoenixctl import log_obs


def _write_events(path: Path, lines: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        "\n".join(json.dumps(x) for x in lines) + "\n", encoding="utf-8"
    )


@pytest.fixture()
def repo(tmp_path: Path) -> Path:
    (tmp_path / "metacache" / "schemas").mkdir(parents=True)
    schema_src = (
        Path(__file__).resolve().parents[3].parent
        / "metacache"
        / "schemas"
        / "obs_event_v1.json"
    )
    if schema_src.is_file():
        (tmp_path / "metacache" / "schemas" / "obs_event_v1.json").write_text(
            schema_src.read_text(encoding="utf-8"), encoding="utf-8"
        )
    return tmp_path


def test_summarize_no_events_file(repo: Path) -> None:
    out = log_obs.summarize(repo, "454107DB", "telemetry")
    assert out["events_status"] == "no_events_file"
    assert out["event_count"] == 0


def test_summarize_empty_file(repo: Path) -> None:
    path = repo / "telemetry" / "454107db_events.jsonl"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("", encoding="utf-8")
    out = log_obs.summarize(repo, "454107DB", "telemetry")
    assert out["events_status"] == "empty"


def test_summarize_malformed_line(repo: Path) -> None:
    path = repo / "telemetry" / "454107db_events.jsonl"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('{"v":1,"code":"ok"}\nnot json\n', encoding="utf-8")
    out = log_obs.summarize(repo, "454107DB", "telemetry")
    assert out["event_count"] == 1
    assert out["invalid_event_count"] == 1


def test_classify_bf2_invariants(repo: Path) -> None:
    _write_events(
        repo / "telemetry" / "454107db_events.jsonl",
        [{"v": 1, "code": "DepthHostSidecarStale", "domain": "Gpu", "ts_ms": 1}],
    )
    out = log_obs.summarize(repo, "454107DB", "telemetry")
    cls = out["classification"]
    assert "U-GPU-001" in cls["finding_ids"]
    assert "G-454107DB-003" in cls["finding_ids"]


def test_classify_bf2_probe_only(repo: Path) -> None:
    probe = {
        "gpu": {
            "host_depth_transfer_mismatch_count": 2,
            "obs_depth_host_sidecar_stale_count": 0,
        }
    }
    out = log_obs.summarize(repo, "454107DB", "telemetry", probe)
    cls = out["classification"]
    assert "G-454107DB-003" in cls["finding_ids"]


def test_non_bf2_no_classification(repo: Path) -> None:
    _write_events(
        repo / "telemetry" / "4d5307d1_events.jsonl",
        [{"v": 1, "code": "KernelStubHit", "domain": "Kernel", "ts_ms": 1}],
    )
    out = log_obs.summarize(repo, "4D5307D1", "telemetry")
    assert "classification" not in out
