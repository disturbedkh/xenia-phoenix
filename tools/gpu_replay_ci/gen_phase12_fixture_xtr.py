#!/usr/bin/env python3
"""Emit Phoenix-authored minimal GPU traces for Phase 1.2 corpus bootstrapping.

Each file is a legal-to-ship synthetic capture: kEvent/kSwap only (one closed frame,
no PM4 execution). These exercise trace load + dump path pairing without retail
game binaries. Replace slots with full captures per CORPUS.md for resolve testing.

Regenerate after trace protocol changes (see kTraceFormatVersion in trace_protocol.h).

Usage (from xenia-phoenix-src root):
  python tools/gpu_replay_ci/gen_phase12_fixture_xtr.py
"""

from __future__ import annotations

import struct
from pathlib import Path

# Must match src/xenia/gpu/trace_protocol.h
TRACE_FORMAT_VERSION = 1

# TraceCommandType ordering (trace_protocol.h)
CMD_PACKET_START = 4
CMD_PACKET_END = 5
CMD_EVENT = 9
EVENT_TYPE_SWAP = 0

# Guest physical base for ringbuffer copy (64 KiB page heap in Xenia memory map).
RING_BASE_PTR = 0xE0000000

# PM4_NOP — safe no-op packet for trace-dump smoke (no VdSwap / presenter required).
PM4_NOP = 0x10


def make_packet_type3(opcode: int, count: int, predicate: bool = False) -> int:
    assert 1 <= count <= 0x4000
    assert opcode <= 0x7F
    return (
        (3 << 30)
        | (((count - 1) & 0x3FFF) << 16)
        | ((opcode & 0x7F) << 8)
        | (1 if predicate else 0)
    )


def build_nop_ringbuffer_be() -> bytes:
    """Type3 PM4_NOP with a single padding dword (minimal legal packet)."""
    header = make_packet_type3(PM4_NOP, 1, False)
    return struct.pack(">I", header) + struct.pack(">I", 0)


def write_trace(path: Path, title_id: int, build_sha40: bytes = b"0" * 40) -> None:
    if len(build_sha40) != 40:
        raise ValueError("build_commit_sha must be 40 bytes ASCII")
    # Event-only trace: closes one frame without executing PM4 (avoids trace-dump
    # crashes on minimal ringbuffer packets in headless environments).
    swap_event = struct.pack("<II", CMD_EVENT, EVENT_TYPE_SWAP)
    header = struct.pack("<I", TRACE_FORMAT_VERSION) + build_sha40 + struct.pack("<I", title_id)
    path.write_bytes(header + swap_event)


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    out_dir = repo / "tests" / "gpu_traces"
    out_dir.mkdir(parents=True, exist_ok=True)

    fixtures = [
        ("phoenix_slot_a_depth_resolve_min.xtr", 0xF0000001),
        ("phoenix_slot_b_msaa_color_min.xtr", 0xF0000002),
        ("phoenix_slot_c_msaa_depth_min.xtr", 0xF0000003),
        ("phoenix_slot_d_tiled_color_resolve_min.xtr", 0xF0000004),
        ("phoenix_slot_e_predicated_viewport_min.xtr", 0xF0000005),
        ("phoenix_slot_f_memexport_stress_min.xtr", 0xF0000006),
        ("phoenix_slot_g_depth_only_min.xtr", 0xF0000007),
        ("phoenix_slot_h_gamma_ramp_min.xtr", 0xF0000008),
        ("phoenix_slot_i_bin_mask_min.xtr", 0xF0000009),
        ("phoenix_slot_j_generic_clear_min.xtr", 0xF000000A),
    ]

    sha40 = b"0" * 40

    for name, tid in fixtures:
        write_trace(out_dir / name, tid, build_sha40=sha40)
        print("wrote", name)
    print(f"Done: {len(fixtures)} traces -> {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
