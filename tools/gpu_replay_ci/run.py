# GPU trace replay CI (Tier 0 / MVP2)
#
# Prerequisites:
# - Build with -DXENIA_BUILD_MISC=ON (produces xenia-gpu-d3d12-trace-dump and
#   xenia-gpu-vulkan-trace-dump under build/bin/Windows).
# - Place one or more `.xtr` captures under tests/gpu_traces/ (see README).
#
# Usage:
#   python tools/gpu_replay_ci/run.py --build-dir build --backend d3d12
#   python tools/gpu_replay_ci/run.py --build-dir build --cross-path d3d12

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path


def sha256_dir(root: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for p in sorted(root.rglob("*")):
        if p.is_file():
            rel = p.relative_to(root).as_posix()
            h = hashlib.sha256()
            with p.open("rb") as f:
                for chunk in iter(lambda: f.read(1024 * 1024), b""):
                    h.update(chunk)
            out[rel] = h.hexdigest()
    return out


def run_dump(bin_path: Path, trace: Path, out_dir: Path, extra_args: list[str]) -> int:
    out_dir.mkdir(parents=True, exist_ok=True)
    cmd = [str(bin_path)]
    cmd += extra_args
    cmd += [f"--target_trace_file={trace.as_posix()}", f"--trace_dump_path={out_dir.as_posix()}"]
    print("+", " ".join(cmd))
    return subprocess.call(cmd, cwd=bin_path.parent)


def main() -> int:
    ap = argparse.ArgumentParser(description="Replay GPU traces and diff dump trees.")
    ap.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    ap.add_argument("--build-dir", type=Path, default=Path("build"))
    ap.add_argument("--traces-dir", type=Path, default=None)
    ap.add_argument("--golden-dir", type=Path, default=None)
    ap.add_argument(
        "--backend",
        choices=("d3d12", "vulkan"),
        default="d3d12",
        help="Which trace-dump host binary to run.",
    )
    ap.add_argument(
        "--cross-path",
        choices=("none", "d3d12"),
        default="none",
        help="If d3d12, run RTV vs ROV and diff hashes (requires D3D12 trace dump).",
    )
    args = ap.parse_args()

    repo = args.repo_root
    traces_dir = args.traces_dir or (repo / "tests" / "gpu_traces")
    golden_root = args.golden_dir or (repo / "tests" / "gpu_traces" / "golden")

    plat_bin = args.build_dir / "bin" / "Windows"
    if args.backend == "d3d12":
        exe = plat_bin / "xenia-gpu-d3d12-trace-dump.exe"
    else:
        exe = plat_bin / "xenia-gpu-vulkan-trace-dump.exe"

    if not exe.is_file():
        print(f"ERROR: missing {exe} — configure with -DXENIA_BUILD_MISC=ON", file=sys.stderr)
        return 2

    traces = sorted(traces_dir.glob("*.xtr"))
    if not traces:
        print(f"No .xtr files under {traces_dir} — add captures or pass --traces-dir.", file=sys.stderr)
        return 0

    report: dict = {"backend": args.backend, "traces": []}

    for trace in traces:
        base_out = args.repo_root / "gpu_replay_out" / trace.stem
        if base_out.exists():
            shutil.rmtree(base_out)
        base_out.mkdir(parents=True)

        if args.cross_path == "d3d12" and args.backend == "d3d12":
            out_rtv = base_out / "rtv"
            out_rov = base_out / "rov"
            rc1 = run_dump(
                exe,
                trace,
                out_rtv,
                ["--render_target_path_d3d12=rtv"],
            )
            rc2 = run_dump(
                exe,
                trace,
                out_rov,
                ["--render_target_path_d3d12=rov"],
            )
            if rc1 or rc2:
                return 1
            h1 = sha256_dir(out_rtv)
            h2 = sha256_dir(out_rov)
            diff_keys = sorted(k for k in set(h1) | set(h2) if h1.get(k) != h2.get(k))
            report["traces"].append(
                {
                    "file": trace.name,
                    "cross_path_rtv_rov_diff_files": diff_keys,
                }
            )
        else:
            out = base_out / "default"
            rc = run_dump(exe, trace, out, [])
            if rc:
                return 1
            report["traces"].append({"file": trace.name, "dump_sha256": sha256_dir(out)})

    golden_root.mkdir(parents=True, exist_ok=True)
    report_path = golden_root / "last_report.json"
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Wrote {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
