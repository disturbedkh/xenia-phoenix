"""Optional JSON Schema validation for obs_event_v1."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

try:
    import jsonschema  # type: ignore
except ImportError:
    jsonschema = None


def schema_path(repo: Path) -> Path | None:
    metacache = repo.parent / "metacache" / "schemas" / "obs_event_v1.json"
    if metacache.is_file():
        return metacache
    return None


def validate_event(obj: dict[str, Any], schema: dict[str, Any]) -> bool:
    if jsonschema is None:
        return True
    try:
        jsonschema.validate(obj, schema)
        return True
    except jsonschema.ValidationError:
        return False


def load_schema(repo: Path) -> dict[str, Any] | None:
    path = schema_path(repo)
    if not path:
        return None
    return json.loads(path.read_text(encoding="utf-8"))
