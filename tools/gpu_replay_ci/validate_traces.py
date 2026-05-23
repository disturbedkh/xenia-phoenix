#!/usr/bin/env python3
"""Validate .xtr files (format version + command stream walk)."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

TRACE_FORMAT_VERSION = 1

# MSVC struct sizes from trace_protocol.h (must match trace_reader.cc).
TRACE_HEADER_BYTES = 48
EVENT_COMMAND_BYTES = 12
REGISTERS_COMMAND_BYTES = 24
REGISTERS_ENCODED_LENGTH_OFFSET = 20

# Must match xe::gpu::TraceCommandType in trace_protocol.h
CMD = (
    "kPrimaryBufferStart",
    "kPrimaryBufferEnd",
    "kIndirectBufferStart",
    "kIndirectBufferEnd",
    "kPacketStart",
    "kPacketEnd",
    "kMemoryRead",
    "kMemoryWrite",
    "kEdramSnapshot",
    "kEvent",
    "kRegisters",
    "kGammaRamp",
)


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def validate_trace(path: Path) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    data = path.read_bytes()
    if len(data) < TRACE_HEADER_BYTES:
        errors.append("file too small for TraceHeader")
        return errors, warnings

    version = u32(data, 0)
    if version != TRACE_FORMAT_VERSION:
        errors.append(f"version {version} != {TRACE_FORMAT_VERSION}")

    off = TRACE_HEADER_BYTES
    while off < len(data):
        if off + 4 > len(data):
            errors.append(f"truncated command at offset {off}")
            break
        cmd_type = u32(data, off)
        if cmd_type >= len(CMD):
            errors.append(f"unknown command type {cmd_type} at {off}")
            break
        name = CMD[cmd_type]

        if name == "kPrimaryBufferStart":
            if off + 12 > len(data):
                errors.append("truncated PrimaryBufferStart")
                break
            count = u32(data, off + 8)
            off += 12 + count * 4
        elif name == "kIndirectBufferStart":
            if off + 12 > len(data):
                errors.append("truncated IndirectBufferStart")
                break
            count = u32(data, off + 8)
            off += 12 + count * 4
        elif name in ("kPrimaryBufferEnd", "kIndirectBufferEnd", "kPacketEnd"):
            off += 4
            if name == "kIndirectBufferEnd":
                # trace_reader expects PacketEnd immediately after IndirectBufferEnd
                if off + 4 > len(data):
                    errors.append("truncated PacketEnd after IndirectBufferEnd")
                    break
                if u32(data, off) != CMD.index("kPacketEnd"):
                    errors.append(
                        f"expected kPacketEnd after kIndirectBufferEnd at {off}"
                    )
                    break
                off += 4
        elif name == "kPacketStart":
            if off + 12 > len(data):
                errors.append("truncated PacketStart")
                break
            count = u32(data, off + 8)
            off += 12 + count * 4
        elif name in ("kMemoryRead", "kMemoryWrite"):
            if off + 20 > len(data):
                errors.append(f"truncated {name}")
                break
            enc_len = u32(data, off + 12)
            off += 20 + enc_len
        elif name == "kEdramSnapshot":
            if off + 12 > len(data):
                errors.append("truncated EdramSnapshot")
                break
            enc_len = u32(data, off + 8)
            off += 12 + enc_len
        elif name == "kEvent":
            if off + EVENT_COMMAND_BYTES > len(data):
                if off < len(data):
                    warnings.append(
                        f"truncated kEvent tail at {off} "
                        f"({len(data) - off} bytes; abrupt emulator exit)"
                    )
                    off = len(data)
                break
            off += EVENT_COMMAND_BYTES
        elif name == "kRegisters":
            if off + REGISTERS_COMMAND_BYTES > len(data):
                errors.append("truncated Registers")
                break
            enc_len = u32(data, off + REGISTERS_ENCODED_LENGTH_OFFSET)
            off += REGISTERS_COMMAND_BYTES + enc_len
        elif name == "kGammaRamp":
            if off + 16 > len(data):
                errors.append("truncated GammaRamp")
                break
            enc_len = u32(data, off + 12)
            off += 16 + enc_len
        else:
            errors.append(f"unhandled command {name}")
            break

    if not errors and off > len(data):
        errors.append(f"parser overrun by {off - len(data)} bytes at offset {off}")
    elif not errors and off < len(data):
        errors.append(f"trailing bytes {len(data) - off} at offset {off}")

    return errors, warnings


def collect_trace_paths(traces: list[Path], traces_dir: Path | None) -> list[Path]:
    paths: list[Path] = list(traces)
    if traces_dir is not None:
        if not traces_dir.is_dir():
            print(f"traces-dir not found: {traces_dir}", file=sys.stderr)
            return []
        paths.extend(sorted(traces_dir.glob("*.xtr")))
    # Stable order, no duplicates.
    seen: set[Path] = set()
    unique: list[Path] = []
    for path in paths:
        resolved = path.resolve()
        if resolved in seen:
            continue
        seen.add(resolved)
        unique.append(path)
    return unique


def main() -> int:
    ap = argparse.ArgumentParser(description="Validate GPU .xtr trace files.")
    ap.add_argument(
        "traces",
        nargs="*",
        type=Path,
        help="Path(s) to .xtr files (optional if --traces-dir is set)",
    )
    ap.add_argument(
        "--traces-dir",
        type=Path,
        default=None,
        help="Directory containing *.xtr files to validate",
    )
    args = ap.parse_args()

    paths = collect_trace_paths(list(args.traces), args.traces_dir)
    if not paths:
        print(
            "No trace files given (pass paths and/or --traces-dir)",
            file=sys.stderr,
        )
        return 1

    failed = 0
    for path in paths:
        errs, warns = validate_trace(path)
        if warns:
            print(f"OK {path} (warnings):")
            for w in warns:
                print(f"  warn: {w}")
        if errs:
            failed += 1
            print(f"FAIL {path}:")
            for e in errs:
                print(f"  {e}")
        elif not warns:
            print(f"OK {path}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
