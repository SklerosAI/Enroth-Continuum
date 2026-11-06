#!/usr/bin/env python3
"""Verify bounded MM6 item-event dispatch addresses against the owner-local PE."""

import argparse
import hashlib
import json
import struct
from pathlib import Path

from mm6_f3_pe_tables import PEImage


def u32(image: PEImage, va: int) -> int:
    return struct.unpack("<I", image.slice(va, 4)[0])[0]


def jump_target(image: PEImage, index: int, index_table: int, target_table: int) -> int:
    entry = image.slice(index_table + index, 1)[0][0]
    return u32(image, target_table + 4 * entry)


def call_target(image: PEImage, va: int) -> int:
    opcode, displacement = struct.unpack("<Bi", image.slice(va, 5)[0])
    if opcode != 0xE8:
        raise ValueError("expected a direct call in MM6 item-event path")
    return va + 5 + displacement


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("f3_report", type=Path)
    args = parser.parse_args()

    exe = args.executable.resolve(strict=True)
    if exe.name.lower() != "mm6.exe":
        parser.error("expected the owner's installed MM6.exe")
    f3 = json.loads(args.f3_report.read_text())
    data = exe.read_bytes()
    if hashlib.sha256(data).hexdigest() != f3["exe_sha256"]:
        raise ValueError("MM6 executable differs from the accepted F3 PE identity")
    image = PEImage(data)
    if image.image_base != 0x400000:
        raise ValueError("unexpected MM6 PE image base")

    opcode_targets = {14: 0x43D05A, 16: 0x43D369, 17: 0x43D195}
    for opcode, expected in opcode_targets.items():
        if u32(image, 0x43E3C8 + (opcode - 1) * 4) != expected:
            raise ValueError("MM6 item opcode dispatcher changed")
    if (jump_target(image, 16, 0x441FF0, 0x441F1C) != 0x441860 or
            jump_target(image, 14, 0x442AE4, 0x442A20) != 0x442693 or
            jump_target(image, 16, 0x440634, 0x440510) != 0x4401A9):
        raise ValueError("MM6 item variable dispatcher changed")
    if (call_target(image, 0x43D3B4) != 0x4412B0 or
            call_target(image, 0x43D1E0) != 0x4420E0 or
            call_target(image, 0x44188C) != 0x487750 or
            call_target(image, 0x4426CD) != 0x45A910 or
            call_target(image, 0x4426DA) != 0x4873F0):
        raise ValueError("MM6 item-event effect call changed")
    print("MM6 item PE dispatch: Add, Subtract, Compare and cursor/grid helpers match F3 identity")


if __name__ == "__main__":
    main()
