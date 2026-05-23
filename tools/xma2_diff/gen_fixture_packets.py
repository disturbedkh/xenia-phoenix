#!/usr/bin/env python3
"""Emit synthetic XMA2 packet fixtures for xma2-diff (legal, in-repo only)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

PACKET = 2048
HEADER = 4


def xma2_first_frame_offset_bits(packet: bytearray) -> int:
    val = ((packet[0] & 3) << 13) | (packet[1] << 5) | (packet[2] >> 3)
    return val + 32


def make_xma2_header(
    frame_count: int,
    first_frame_bits: int,
    skip: int = 0,
) -> bytearray:
    pkt = bytearray(PACKET)
    off = max(32, first_frame_bits) - 32
    pkt[0] = ((frame_count & 0x3F) << 2) | ((off >> 13) & 3)
    pkt[1] = (off >> 5) & 0xFF
    pkt[2] = ((off & 0x1F) << 3) | 1  # XMA2 metadata = 1
    pkt[3] = skip & 0xFF
    return pkt


def make_full_skip_packet() -> bytearray:
    pkt = bytearray(PACKET)
    pkt[2] = 1
    pkt[3] = 0xFF
    return pkt


def make_minimal_frame_packet(frame_size_bits: int = 128) -> bytearray:
    """Single-frame XMA2 packet with a tiny zero bitstream after the header."""
    pkt = make_xma2_header(frame_count=1, first_frame_bits=32)
    # 15-bit frame header at bit offset 32 (byte 4+): size in bits
    bit_off = 32
    byte_off = bit_off // 8
    bit_in_byte = bit_off % 8
    # Pack 15-bit size little-endian across bytes (bitstream order)
    size_val = frame_size_bits & 0x7FFF
    for i in range(15):
        bit = (size_val >> (14 - i)) & 1
        pos = byte_off + (bit_in_byte + i) // 8
        bbit = (bit_in_byte + i) % 8
        if pos < PACKET:
            if bit:
                pkt[pos] |= 1 << (7 - bbit)
    return pkt


FIXTURES: list[dict] = [
    {
        "id": "mono_48k_full_skip",
        "mode": "structural",
        "sample_rate_id": 3,
        "sample_rate_hz": 48000,
        "channels": 1,
        "is_stereo": 0,
        "packet_count": 1,
        "builder": make_full_skip_packet,
    },
    {
        "id": "stereo_44k_full_skip",
        "mode": "structural",
        "sample_rate_id": 2,
        "sample_rate_hz": 44100,
        "channels": 2,
        "is_stereo": 1,
        "packet_count": 1,
        "builder": make_full_skip_packet,
    },
    {
        "id": "mono_32k_header_clamp",
        "mode": "structural",
        "sample_rate_id": 1,
        "sample_rate_hz": 32000,
        "channels": 1,
        "is_stereo": 0,
        "packet_count": 1,
        "read_offset_bits": 0,
        "builder": lambda: make_xma2_header(0, 32, skip=0),
    },
    {
        "id": "mono_24k_loop_meta",
        "mode": "structural",
        "sample_rate_id": 0,
        "sample_rate_hz": 24000,
        "channels": 1,
        "is_stereo": 0,
        "packet_count": 1,
        "loop_count": 2,
        "builder": make_full_skip_packet,
    },
    {
        "id": "stereo_48k_min_frame",
        "mode": "structural",
        "allow_pcm": True,
        "sample_rate_id": 3,
        "sample_rate_hz": 48000,
        "channels": 2,
        "is_stereo": 1,
        "packet_count": 1,
        "builder": lambda: make_minimal_frame_packet(256),
    },
    {
        "id": "mono_48k_min_frame",
        "mode": "structural",
        "allow_pcm": True,
        "sample_rate_id": 3,
        "sample_rate_hz": 48000,
        "channels": 1,
        "is_stereo": 0,
        "packet_count": 1,
        "builder": lambda: make_minimal_frame_packet(256),
    },
    {
        "id": "mono_48k_two_packet_skip",
        "mode": "structural",
        "sample_rate_id": 3,
        "sample_rate_hz": 48000,
        "channels": 1,
        "is_stereo": 0,
        "packet_count": 2,
        "builder": lambda: make_full_skip_packet() + make_full_skip_packet(),
    },
]


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    out_dir = repo / "tests" / "xma2_packets"
    out_dir.mkdir(parents=True, exist_ok=True)

    for spec in FIXTURES:
        fid = spec["id"]
        packets = spec["builder"]()
        if isinstance(packets, bytearray):
            blob = packets
        else:
            blob = packets
        if spec.get("packet_count", 1) > 1 and len(blob) == PACKET:
            # builder returned single packet but count>1 — already concatenated?
            pass
        xma_path = out_dir / f"{fid}.xma"
        xma_path.write_bytes(blob)

        meta = {
            "id": fid,
            "mode": spec.get("mode", "structural"),
            "sample_rate_id": spec["sample_rate_id"],
            "sample_rate_hz": spec["sample_rate_hz"],
            "channels": spec["channels"],
            "is_stereo": spec["is_stereo"],
            "packet_count": spec.get("packet_count", len(blob) // PACKET),
        }
        if "read_offset_bits" in spec:
            meta["read_offset_bits"] = spec["read_offset_bits"]
        if "loop_count" in spec:
            meta["loop_count"] = spec["loop_count"]
        if spec.get("allow_pcm"):
            meta["allow_pcm"] = True
        meta["first_frame_offset_bits"] = xma2_first_frame_offset_bits(
            bytearray(blob[:PACKET])
        )
        (out_dir / f"{fid}.json").write_text(
            json.dumps(meta, indent=2) + "\n", encoding="utf-8"
        )
        print(f"wrote {fid} ({len(blob)} bytes)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
