#!/usr/bin/env python3
"""Check retained MM6 item icon footprints without printing proprietary names."""

from __future__ import annotations

import argparse
import csv
import struct
from pathlib import Path


def inventory_slots(pixels: int) -> int:
    return ((max(pixels, 14) - 14) >> 5) + 1


def check(retained_icons: Path) -> tuple[int, int, int, int]:
    with (retained_icons / "items.txt").open(encoding="latin1", newline="") as stream:
        rows = list(csv.reader(stream, delimiter="\t"))

    pngs = {path.stem.lower(): path for path in retained_icons.glob("*.png")}
    seen: set[int] = set()
    usable = 0
    placeholders = 0
    max_width = 0
    max_height = 0
    for row in rows[3:]:
        try:
            item_id = int(row[0])
        except (IndexError, ValueError):
            continue
        if not 1 <= item_id <= 580:
            continue
        if item_id in seen:
            raise ValueError("duplicate MM6 item ID")
        seen.add(item_id)
        icon = row[1].strip('"').lower()
        if not icon:
            placeholders += 1
            continue
        path = pngs.get(icon)
        if path is None:
            raise ValueError("missing retained MM6 item icon")
        with path.open("rb") as stream:
            header = stream.read(24)
        if header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
            raise ValueError("invalid retained MM6 icon PNG")
        pixels_w, pixels_h = struct.unpack(">II", header[16:24])
        slots_w, slots_h = inventory_slots(pixels_w), inventory_slots(pixels_h)
        if not 1 <= slots_w <= 14 or not 1 <= slots_h <= 9:
            raise ValueError("MM6 item footprint exceeds character grid")
        max_width = max(max_width, slots_w)
        max_height = max(max_height, slots_h)
        usable += 1

    if len(seen) != 580 or usable != 578 or placeholders != 2:
        raise ValueError("MM6 item table/icon coverage changed")
    return usable, placeholders, max_width, max_height


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("retained_icons", type=Path, help="ignored retained icons directory")
    args = parser.parse_args()
    usable, placeholders, max_width, max_height = check(args.retained_icons)
    print(f"MM6 item footprints: {usable} usable icons, {placeholders} blank rows, "
          f"maximum {max_width}x{max_height} slots")


if __name__ == "__main__":
    main()
