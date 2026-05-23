#!/usr/bin/env python3
"""Join patch_debt_dashboard_data.json with subsystem hints for smoke-phase triage."""

from __future__ import annotations

import json
import re
from collections import Counter, defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_PATCH_JSON = REPO / "docs" / "patch_debt_dashboard_data.json"
OUT_MD = REPO / "docs" / "patch_debt_triage.md"

# Heuristic reclass: cosmetic/QoL patches often mis-tagged as C.
FALSE_POSITIVE_C_KEYWORDS = (
    "button prompt",
    "ps3 button",
    "unlock fps",
    "show fps",
    "widescreen",
    "fov",
    "brightness",
    "disable lens",
    "disable motion blur",
    "disable dof",
    "hd shadows",
    "anisotropic",
)

SUBSYSTEM_RULES: list[tuple[str, re.Pattern[str]]] = [
    ("gpu", re.compile(r"shadow|resolve|edram|render|blur|dof|flare|zpass|gpu", re.I)),
    ("apu", re.compile(r"audio|xma|sound|music|apu", re.I)),
    ("cpu", re.compile(r"vmx|jit|cpu|combo|fps unlock", re.I)),
    ("kernel", re.compile(r"kernel|xam|stub|mount|ioctl|xboxkrnl", re.I)),
]


def guess_subsystem(title: str, patch_name: str) -> str:
    blob = f"{title} {patch_name}"
    for name, pat in SUBSYSTEM_RULES:
        if pat.search(blob):
            return name
    return "unknown"


def is_false_positive_c(patch_name: str) -> bool:
    low = patch_name.lower()
    return any(k in low for k in FALSE_POSITIVE_C_KEYWORDS)


def main() -> int:
    data = json.loads(DEFAULT_PATCH_JSON.read_text(encoding="utf-8"))
    rows = data.get("rows", [])
    cd_rows = [r for r in rows if r.get("category_guess") in ("C", "D")]

    by_title: dict[str, list[dict]] = defaultdict(list)
    for r in cd_rows:
        fname = r.get("file", "")
        tid = fname.split(" - ")[0].strip() if " - " in fname else fname[:8]
        by_title[tid].append(r)

    top = sorted(by_title.items(), key=lambda x: -len(x[1]))[:25]

    lines: list[str] = [
        "# Patch debt triage (PC inventory)",
        "",
        f"Source: `{DEFAULT_PATCH_JSON.relative_to(REPO).as_posix()}`",
        "",
        "## Counts",
        "",
        "```json",
        json.dumps(data.get("counts_by_category", {}), indent=2),
        "```",
        "",
        "## Top C/D titles (heuristic subsystem)",
        "",
        "| Title ID | C+D rows | Subsystem guess | Sample patch |",
        "|----------|----------|-----------------|--------------|",
    ]
    for tid, items in top:
        sample = items[0].get("patch", "")[:48]
        sub = guess_subsystem(items[0].get("title", ""), sample)
        lines.append(f"| {tid} | {len(items)} | {sub} | {sample} |")

    fp_count = sum(1 for r in cd_rows if r.get("category_guess") == "C" and is_false_positive_c(r.get("patch", "")))
    lines.extend(
        [
            "",
            f"## Likely false-positive category C (~{fp_count} rows)",
            "",
            "Cosmetic/QoL keywords — verify on hardware before gameplay retirement:",
            "",
        ]
    )
    for kw in FALSE_POSITIVE_C_KEYWORDS:
        lines.append(f"- `{kw}`")

    lines.extend(
        [
            "",
            "## Gameplay phase",
            "",
            "Per title: disable patch → smoke capture (stub/XMA logs) → fix → delete.",
            "See `Xenia-Phoenix/metacache/plan/61_tier1_gameplay_gate.md`.",
            "",
        ]
    )

    OUT_MD.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {OUT_MD} ({len(cd_rows)} C+D rows, {len(top)} titles in top table)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
