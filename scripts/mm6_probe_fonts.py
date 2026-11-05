#!/usr/bin/env python3
"""Validate retained MM6 font headers, atlases and glyph bounds offline.

The two supported layouts follow Library/LodFormats/LodFormats.cpp. This
metadata-only report must stay in the ignored MM6 research directory.
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path


def valid_glyphs(data: bytes, first: int, last: int, height: int, fmt: str) -> bool:
    atlas_bytes = 4096 if fmt == "mm7_metrics" else 1280
    pixels = len(data) - 32 - atlas_bytes
    if pixels < 0:
        return False
    for char in range(first, last + 1):
        if fmt == "mm7_metrics":
            left, width, right = struct.unpack_from("<iii", data, 32 + 12 * char)
            offset = struct.unpack_from("<I", data, 32 + 3072 + 4 * char)[0]
            if left > 63 or right > 63:
                return False
        else:
            width = data[32 + char]
            offset = struct.unpack_from("<I", data, 32 + 256 + 4 * char)[0]
        if not 1 <= width <= 63 or offset + width * height > pixels:
            return False
    return True


def probe(path: Path) -> dict:
    data = path.read_bytes()
    entry = {"name": path.name, "bytes": len(data),
             "sha256": hashlib.sha256(data).hexdigest()}
    if len(data) < 32 + 1280:
        entry["error"] = "shorter than the minimum font header and atlas"
        return entry
    first, last, marker, zero1, zero2, height, zero3, zero4 = data[:8]
    palette_count = struct.unpack_from("<I", data, 8)[0]
    palettes = struct.unpack_from("<5I", data, 12)
    if not (first < last and marker == 8 and zero1 == zero2 == zero3 == zero4 == 0
            and 4 <= height <= 63 and palette_count == 0 and all(x == 0 for x in palettes)):
        entry["error"] = "invalid font header"
        return entry
    entry.update(first_character=first, last_character=last, height=height)
    for fmt in ("mm7_metrics", "mmx_widths"):
        if valid_glyphs(data, first, last, height, fmt):
            entry["atlas_format"] = fmt
            entry["glyph_count"] = last - first + 1
            return entry
    entry["error"] = "neither supported atlas has valid glyph bounds"
    return entry


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="JSON report inside reference directory")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents:
        parser.error("report must remain in the private reference directory")
    fonts = sorted((root / "icons").glob("*.fnt"))
    entries = [probe(path) for path in fonts]
    failures = [e["name"] for e in entries if "error" in e]
    formats = {fmt: sum(e.get("atlas_format") == fmt for e in entries)
               for fmt in ("mm7_metrics", "mmx_widths")}
    result = {"format": 1, "fonts": entries,
              "summary": {"files": len(entries), "boundary_failures": failures,
                          "atlas_formats": formats}}
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Checked {len(entries)} MM6 fonts; {len(failures)} invalid glyph atlases")
    if len(entries) != 14 or failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
