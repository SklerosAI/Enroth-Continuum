#!/usr/bin/env python3
"""Check retained MM6 outdoor map and delta boundaries without starting the game.

The ODM layout follows src/Engine/Snapshots/MM6OutdoorSnapshot.cpp, checked
against MM8LevelEditor's MM6 OutdoorDataMap and D3Object readers. The DDM
layout follows its DeltaOutdoorDataMap reader:
https://github.com/angeld29/mm8leveleditor/tree/master/src/org/gamenet/application/mm8leveleditor/data/mm6/outdoor

Only ignored, local Games.lod extracts are read. The report records counts,
offsets, hashes and errors, not game content, and must remain in that folder.
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path

from mm6_probe_indoors import Cursor, FormatError, decode


def read_count(cur: Cursor, name: str, record_size: int, entry: dict) -> int:
    cur.count(name, record_size, entry)
    return entry[name]


def probe_base(path: Path) -> dict:
    entry: dict = {"name": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
    try:
        data, checksum_valid, trailing_bytes = decode(path)
        entry.update(decoded_sha256=hashlib.sha256(data).hexdigest(),
                     decoded_bytes=len(data), checksum_valid=checksum_valid,
                     trailing_payload_bytes=trailing_bytes)
        cur = Cursor(data)
        # 160-byte string header, four 4-byte tile selectors, three 128x128 maps.
        cur.take(160 + 16 + 3 * 128 * 128)

        if len(data) - cur.offset < 4:
            raise FormatError("missing outdoor model count")
        model_count = struct.unpack_from("<I", data, cur.offset)[0]
        if model_count > (len(data) - cur.offset - 4) // 188:
            raise FormatError("outdoor model headers overrun the file")
        models = []
        for index in range(model_count):
            start = cur.offset + 4 + index * 188
            vertices = struct.unpack_from("<I", data, start + 68)[0]
            faces = struct.unpack_from("<I", data, start + 76)[0]
            bsp_nodes = struct.unpack_from("<I", data, start + 92)[0]
            models.append((vertices, faces, bsp_nodes))
        cur.take(4 + model_count * 188)
        entry["models"] = model_count
        entry["model_vertices"] = sum(model[0] for model in models)
        entry["model_faces"] = sum(model[1] for model in models)
        entry["model_bsp_nodes"] = sum(model[2] for model in models)
        for vertices, faces, bsp_nodes in models:
            # MM6 stores two 12-byte BSP node lists per model. The existing
            # runtime adapter currently skips them rather than preserving them.
            cur.take(vertices * 12 + faces * (308 + 2 + 10) + bsp_nodes * 2 * 12)

        decorations = read_count(cur, "decorations", 28, entry)
        cur.take(decorations * 32)  # decoration names
        read_count(cur, "decoration_pids", 2, entry)
        cur.take(128 * 128 * 4)  # fixed decoration map
        read_count(cur, "spawn_points", 20, entry)
        entry["parsed_bytes"] = cur.offset
        if cur.offset != len(data):
            raise FormatError(f"parsed {cur.offset} of {len(data)} decoded bytes")
    except (FormatError, OSError, struct.error) as exc:
        entry["error"] = str(exc)
    return entry


def probe_delta(path: Path) -> dict:
    entry: dict = {"name": path.name}
    try:
        if not path.is_file():
            raise FormatError("delta companion is missing")
        entry["sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
        data, checksum_valid, trailing_bytes = decode(path)
        entry.update(decoded_sha256=hashlib.sha256(data).hexdigest(),
                     decoded_bytes=len(data), checksum_valid=checksum_valid,
                     trailing_payload_bytes=trailing_bytes)
        cur = Cursor(data)
        cur.take(8)  # reset count and last reset day
        cur.take(2 * 88 * 11)  # fully and partly revealed map cells
        # MM6 has no MM7 per-face or per-sprite attribute arrays here.
        read_count(cur, "creatures", 548, entry)
        read_count(cur, "items", 100, entry)
        read_count(cur, "chests", 4204, entry)  # 140 items and 140 grid cells
        cur.take(200)  # persistent map bits
        cur.take(56)  # visit time, sky, five four-byte fields, reserved bytes
        entry["parsed_bytes"] = cur.offset
        if cur.offset != len(data):
            raise FormatError(f"parsed {cur.offset} of {len(data)} decoded bytes")
    except (FormatError, OSError, struct.error) as exc:
        entry["error"] = str(exc)
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
    games = root / "Games"
    paths = sorted(games.glob("*.odm"))
    if not paths:
        parser.error("no retained outdoor maps found")
    entries = [probe_base(path) for path in paths]
    deltas = [probe_delta(path.with_suffix(".ddm")) for path in paths]
    failures = [entry["name"] for entry in entries + deltas if "error" in entry]
    result = {
        "format": 1,
        "source": "retained MM6 Games.lod outdoor and delta extracts",
        "maps": entries,
        "deltas": deltas,
        "summary": {"base_maps": len(entries), "deltas": len(deltas),
                    "layout_failures": failures,
                    "checksum_failures": [e["name"] for e in entries + deltas if e.get("checksum_valid") is False]},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Probed {len(entries)} outdoor maps and {len(deltas)} deltas; {len(failures)} layout failures")
    for entry in entries + deltas:
        if "error" in entry:
            print(f"  {entry['name']}: {entry['error']}")
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
