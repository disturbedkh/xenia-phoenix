import json
from pathlib import Path

from phoenixctl.paths import title_paths
from phoenixctl.session import (
    clear_session,
    pid_alive,
    read_session,
    start_session_record,
    write_session,
)


def test_session_round_trip(tmp_path, monkeypatch):
    monkeypatch.setenv("PHOENIX_SRC", str(tmp_path))
    (tmp_path / "tools" / "tier0").mkdir(parents=True)
    (tmp_path / "src" / "xenia").mkdir(parents=True)
    paths = title_paths(tmp_path, "454107DB")
    data = start_session_record(
        paths, pid=99999, mode="triage", game_path=r"D:\Games\secret\game.iso"
    )
    assert data["game_basename"] == "game.iso"
    assert "secret" not in json.dumps(data)
    loaded = read_session(paths.session_file)
    assert loaded["title_id"] == "454107db"
    clear_session(paths.session_file)
    assert read_session(paths.session_file) is None


def test_pid_alive():
    assert pid_alive(0) is False
