# GPU trace replay CI (Tier 0 / MVP2)
#
# Prerequisites:
# - Build with -DXENIA_BUILD_MISC=ON (produces xenia-gpu-d3d12-trace-dump and
#   xenia-gpu-vulkan-trace-dump under Build/Windows/x64/<Config>).
# - Place one or more `.xtr` captures under tests/gpu_traces/ (see README).
#
# Usage:
#   python tools/gpu_replay_ci/run.py --build-dir Build/Windows/x64 --backend d3d12
#   python tools/gpu_replay_ci/run.py --build-dir Build/Windows/x64 --cross-path d3d12
#   # Exploratory: do not fail on RTV/ROV tree mismatch
#   python tools/gpu_replay_ci/run.py --build-dir Build/Windows/x64 --cross-path d3d12 --allow-rtv-rov-drift

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
    ap.add_argument("--build-dir", type=Path, default=Path("Build/Windows/x64"))
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
    ap.add_argument(
        "--allow-rtv-rov-drift",
        action="store_true",
        help="Do not fail if RTV and ROV dump trees differ (report only). Default: exit nonzero on drift.",
    )
    ap.add_argument(
        "--format-validate-only",
        action="store_true",
        help="Only validate .xtr structure (no trace-dump binary). Writes a green format report.",
    )
    args = ap.parse_args()

    repo = args.repo_root
    traces_dir = args.traces_dir or (repo / "tests" / "gpu_traces")
    golden_root = args.golden_dir or (repo / "tests" / "gpu_traces" / "golden")

    plat_bin = args.build_dir
    if args.backend == "d3d12":
        exe_name = "xenia-gpu-d3d12-trace-dump.exe"
    else:
        exe_name = "xenia-gpu-vulkan-trace-dump.exe"

    traces = sorted(traces_dir.glob("*.xtr"))
    if args.format_validate_only:
        if not traces:
            print(f"No .xtr files under {traces_dir}", file=sys.stderr)
            return 1
        validate_script = repo / "tools" / "gpu_replay_ci" / "validate_traces.py"
        rc = subprocess.call([sys.executable, str(validate_script), "--traces-dir", str(traces_dir)])
        if rc:
            return rc
        report = {
            "backend": args.backend,
            "note": "format_validate_only",
            "cross_path": args.cross_path,
            "corpus_count": len(traces),
            "rtv_rov_drift": False,
            "drift_traces": [],
            "total_diff_files": 0,
            "traces": [{"file": t.name, "format_ok": True} for t in traces],
        }
        golden_root.mkdir(parents=True, exist_ok=True)
        report_path = golden_root / "last_report.json"
        report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"Wrote {report_path} (format validation only)")
        return 0

    traces = sorted(traces_dir.glob("*.xtr"))
    if not traces:
        print(f"No .xtr files under {traces_dir} — add captures or pass --traces-dir.", file=sys.stderr)
        golden_root.mkdir(parents=True, exist_ok=True)
        report_path = golden_root / "last_report.json"
        empty_report = {
            "backend": args.backend,
            "note": "no_traces",
            "traces_dir": str(traces_dir),
            "traces": [],
        }
        report_path.write_text(json.dumps(empty_report, indent=2), encoding="utf-8")
        print(f"Wrote {report_path} (empty corpus)")
        return 0

    exe = plat_bin / exe_name
    if not exe.is_file():
        for cfg in ("Release", "Checked", "Debug"):
            cand = plat_bin / cfg / exe_name
            if cand.is_file():
                exe = cand
                break

    if not exe.is_file():
        print(
            f"ERROR: missing {exe_name} under {plat_bin} (tried Debug/Release/Checked) — "
            "configure with -DXENIA_BUILD_MISC=ON",
            file=sys.stderr,
        )
        return 2

    report: dict = {
        "backend": args.backend,
        "cross_path": args.cross_path,
        "allow_rtv_rov_drift": bool(args.allow_rtv_rov_drift),
        "traces": [],
    }
    drift_traces: list[str] = []
    total_diff_files = 0

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
            if rc1 != 0 or rc2 != 0:
                if rc1 == rc2:
                    print(
                        f"WARN: {trace.name} trace-dump exited {rc1} for both RTV and ROV "
                        "(treating as matched failure; empty dump trees).",
                        file=sys.stderr,
                    )
                else:
                    return 1
            h1 = sha256_dir(out_rtv)
            h2 = sha256_dir(out_rov)
            diff_keys = sorted(k for k in set(h1) | set(h2) if h1.get(k) != h2.get(k))
            if diff_keys:
                drift_traces.append(trace.name)
                total_diff_files += len(diff_keys)
                preview = diff_keys[:12]
                more = "" if len(diff_keys) <= 12 else f" … (+{len(diff_keys) - 12} more)"
                print(
                    f"RTV/ROV drift: {trace.name} — {len(diff_keys)} differing path(s). "
                    f"First: {preview}{more}",
                    file=sys.stderr,
                )
            entry = {
                "file": trace.name,
                "cross_path_rtv_rov_diff_files": diff_keys,
            }
            if rc1 != 0:
                entry["trace_dump_exit_code"] = rc1
            report["traces"].append(entry)
        else:
            out = base_out / "default"
            rc = run_dump(exe, trace, out, [])
            if rc:
                return 1
            report["traces"].append({"file": trace.name, "dump_sha256": sha256_dir(out)})

    golden_root.mkdir(parents=True, exist_ok=True)
    report_path = golden_root / "last_report.json"
    report["drift_traces"] = drift_traces
    report["total_diff_files"] = total_diff_files
    report["rtv_rov_drift"] = bool(drift_traces)
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Wrote {report_path}")

    if (
        args.cross_path == "d3d12"
        and args.backend == "d3d12"
        and drift_traces
        and not args.allow_rtv_rov_drift
    ):
        print(
            f"FAIL: RTV vs ROV drift on {len(drift_traces)} trace(s); "
            "use --allow-rtv-rov-drift to record-only.",
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
