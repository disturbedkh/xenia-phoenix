"""Load games.local.yaml from metacache (gitignored)."""

from __future__ import annotations

from pathlib import Path
from typing import Any

import yaml

from phoenixctl.paths import find_metacache_root, find_repo_root, normalize_title_id


def games_registry_path(repo: Path | None = None) -> Path | None:
    root = find_repo_root() if repo is None else repo
    meta = find_metacache_root(root)
    if not meta:
        return None
    path = meta / "cache" / "games.local.yaml"
    return path if path.is_file() else None


def load_games(repo: Path | None = None) -> dict[str, str]:
    path = games_registry_path(repo)
    if not path:
        return {}
    data: Any = yaml.safe_load(path.read_text(encoding="utf-8"))
    if not data or not isinstance(data, dict):
        return {}
    games = data.get("games") or data
    if not isinstance(games, dict):
        return {}
    out: dict[str, str] = {}
    for k, v in games.items():
        if isinstance(v, str) and v.strip():
            out[normalize_title_id(str(k))] = v.strip()
        elif isinstance(v, dict) and v.get("path"):
            out[normalize_title_id(str(k))] = str(v["path"]).strip()
    return out


def resolve_game_path(title_id: str, game: str | None, repo: Path | None = None) -> str:
    if game:
        return game
    tid = normalize_title_id(title_id)
    reg = load_games(repo)
    if tid not in reg:
        raise RuntimeError(
            f"No --game path and title {tid} not in games.local.yaml. "
            "Copy metacache/cache/games.local.yaml.example and set your paths."
        )
    return reg[tid]
