import os
from pathlib import Path

import pytest

from phoenixctl.paths import find_repo_root, normalize_title_id, title_paths


def test_normalize_title_id():
    assert normalize_title_id("4D5307D1") == "4d5307d1"
    assert normalize_title_id("0x454107DB") == "454107db"


def test_normalize_invalid():
    with pytest.raises(ValueError):
        normalize_title_id("abc")


def test_title_paths():
    repo = find_repo_root()
    p = title_paths(repo, "4D5307D1")
    assert p.stubs.name == "4d5307d1_stubs.jsonl"
    assert p.session_file.name == ".phoenix_session.json"


def test_find_repo_root_env(monkeypatch):
    repo = find_repo_root()
    monkeypatch.setenv("PHOENIX_SRC", str(repo))
    assert find_repo_root() == repo.resolve()
