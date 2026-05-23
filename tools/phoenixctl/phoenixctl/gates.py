"""Tier 0 offline gate wrappers."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from phoenixctl.paths import find_repo_root


def gate_vmx128(iters: int = 10000, seed: int = 42, config: str = "Release") -> tuple[int, str]:
    repo = find_repo_root()
    fuzz = repo / "build" / "bin" / "Windows" / config / "vmx128-fuzz.exe"
    if not fuzz.is_file():
        return 1, f"Missing {fuzz}"
    proc = subprocess.run(
        [
            str(fuzz),
            f"--vmx128_fuzz_iters={iters}",
            f"--vmx128_fuzz_seed={seed}",
        ],
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return proc.returncode, (proc.stdout or "") + (proc.stderr or "")


def gate_gpu_replay(config: str = "Release") -> tuple[int, str]:
    repo = find_repo_root()
    script = repo / "tools" / "gpu_replay_ci" / "run.py"
    build = repo / "build" / "bin" / "Windows" / config
    traces = repo / "tests" / "gpu_traces"
    report = repo / "build" / "replay_report"
    if not script.is_file():
        return 1, f"Missing {script}"
    proc = subprocess.run(
        [
            sys.executable,
            str(script),
            "--xenia-build",
            str(build),
            "--traces",
            str(traces),
            "--report-dir",
            str(report),
        ],
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=3600,
    )
    return proc.returncode, (proc.stdout or "") + (proc.stderr or "")


def gate_patch_debt() -> tuple[int, str]:
    repo = find_repo_root()
    script = repo / "tools" / "tier0" / "categorize_patches.py"
    patches = repo / "game-patches"
    out = repo / "build" / "patch_debt.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run(
        [
            sys.executable,
            str(script),
            "--patches-dir",
            str(patches),
            "--output",
            str(out),
        ],
        cwd=str(repo),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    msg = (proc.stdout or "") + (proc.stderr or "")
    if out.is_file():
        msg += f"\nWrote {out.relative_to(repo)}"
    return proc.returncode, msg
