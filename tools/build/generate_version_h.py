#!/usr/bin/env python3
"""Write build/<dir>/version.h using xenia-build.py git metadata."""

import importlib.util
import os
import sys

_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_SPEC = importlib.util.spec_from_file_location(
    "xenia_build", os.path.join(_ROOT, "xenia-build.py")
)
_MODULE = importlib.util.module_from_spec(_SPEC)
assert _SPEC.loader is not None
_SPEC.loader.exec_module(_MODULE)


def main() -> int:
    build_dir = sys.argv[1] if len(sys.argv) > 1 else "build"
    _MODULE.generate_version_h(build_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
