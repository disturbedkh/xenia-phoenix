"""Repository root and per-title telemetry path resolution."""

from __future__ import annotations

import os
import re
from dataclasses import dataclass
from pathlib import Path


def find_repo_root(start: Path | None = None) -> Path:
    """Resolve xenia-phoenix-src root from env or walk-up."""
    env = os.environ.get("PHOENIX_SRC")
    if env:
        root = Path(env).resolve()
        if (root / "tools" / "tier0").is_dir():
            return root
    cur = (start or Path(__file__)).resolve()
    if cur.is_file():
        cur = cur.parent
    for parent in [cur, *cur.parents]:
        if (parent / "tools" / "tier0").is_dir() and (parent / "src" / "xenia").is_dir():
            return parent
    raise RuntimeError(
        "Cannot find xenia-phoenix-src root. Set PHOENIX_SRC or run from the repo."
    )


def normalize_title_id(title_id: str) -> str:
    s = title_id.strip().upper().replace("0X", "")
    if not re.fullmatch(r"[0-9A-F]{8}", s):
        raise ValueError(f"Invalid title_id (need 8 hex digits): {title_id!r}")
    return s.lower()


@dataclass(frozen=True)
class TitlePaths:
    title_id: str
    telemetry_dir: Path
    stubs: Path
    xma: Path
    pcm: Path
    crash: Path
    gpu_trace: Path
    triage_report: Path
    stub_summary: Path
    pcm_baseline: Path
    session_file: Path
    events: Path
    obs_summary: Path

    def as_dict(self, repo: Path) -> dict[str, str]:
        def rel(p: Path) -> str:
            try:
                return p.relative_to(repo).as_posix()
            except ValueError:
                return p.as_posix()

        return {
            "title_id": self.title_id,
            "telemetry_dir": rel(self.telemetry_dir),
            "stubs": rel(self.stubs),
            "xma": rel(self.xma),
            "pcm": rel(self.pcm),
            "crash": rel(self.crash),
            "gpu_trace": rel(self.gpu_trace),
            "triage_report": rel(self.triage_report),
            "stub_summary": rel(self.stub_summary),
            "pcm_baseline": rel(self.pcm_baseline),
            "session_file": rel(self.session_file),
            "events": rel(self.events),
            "obs_summary": rel(self.obs_summary),
        }


def title_paths(repo: Path, title_id: str, telemetry_dir: str = "telemetry") -> TitlePaths:
    tid = normalize_title_id(title_id)
    tdir = repo / telemetry_dir
    return TitlePaths(
        title_id=tid,
        telemetry_dir=tdir,
        stubs=tdir / f"{tid}_stubs.jsonl",
        xma=tdir / f"{tid}_xma.jsonl",
        pcm=tdir / f"{tid}_pcm.jsonl",
        crash=tdir / f"{tid}_crash.log",
        gpu_trace=tdir / f"{tid}_gpu_trace",
        triage_report=tdir / f"{tid}_triage_report.txt",
        stub_summary=tdir / f"{tid}_stub_summary.json",
        pcm_baseline=tdir / f"{tid}_pcm_baseline.jsonl",
        session_file=tdir / ".phoenix_session.json",
        events=tdir / f"{tid}_events.jsonl",
        obs_summary=tdir / f"{tid}_obs_summary.json",
    )


def find_metacache_root(repo: Path) -> Path | None:
    """Phoenix metacache sibling of xenia-phoenix-src."""
    candidate = repo.parent / "metacache"
    if (candidate / "INDEX.md").is_file():
        return candidate
    env = os.environ.get("PHOENIX_METACACHE")
    if env:
        p = Path(env).resolve()
        if (p / "INDEX.md").is_file():
            return p
    return None
