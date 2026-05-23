#!/usr/bin/env python3
"""Emit docs/kernel_stub_inventory.json from DECLARE_* kStub exports."""

from __future__ import annotations

import json
import re
from pathlib import Path

PATTERN = re.compile(
    r"DECLARE_(XBOXKRNL|XAM|XBDM)_EXPORT\d*\((\w+),",
    re.MULTILINE,
)

STUB_TAG_PATTERN = re.compile(
    r"DECLARE_(?:XBOXKRNL|XAM|XBDM)_EXPORT\d*\([^)]+kStub",
    re.MULTILINE,
)


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    kernel = repo / "src" / "xenia" / "kernel"
    rows: list[dict] = []

    for path in sorted(kernel.rglob("*.cc")):
        text = path.read_text(encoding="utf-8", errors="replace")
        if "kStub" not in text:
            continue
        for match in STUB_TAG_PATTERN.finditer(text):
            block_start = match.start()
            line_start = text.rfind("\n", 0, block_start) + 1
            line_end = text.find("\n", match.end())
            line = text[line_start:line_end]
            m2 = PATTERN.search(line)
            if not m2:
                continue
            module_kind, export_name = m2.group(1).lower(), m2.group(2)
            module = (
                "xboxkrnl"
                if module_kind == "xboxkrnl"
                else ("xam" if module_kind == "xam" else "xbdm")
            )
            rel = path.relative_to(repo).as_posix()
            rows.append(
                {
                    "module": module,
                    "export": export_name,
                    "file": rel,
                }
            )

    out = repo / "docs" / "kernel_stub_inventory.json"
    payload = {
        "generated_by": "tools/tier0/gen_kernel_stub_inventory.py",
        "count": len(rows),
        "exports": rows,
    }
    out.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(rows)} stub exports to {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
