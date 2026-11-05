#!/usr/bin/env python3
"""Check the section boundaries of private MM6 indoor maps offline.

The record sizes and section order are cross-checked against the MM6 branch of
MM8LevelEditor's IndoorDataMap, DeltaIndoorDataMap, IndoorFace, Room, Sprite,
LightSource, SpawnPoint, Creature, Item, ChestContents and Door readers:
https://github.com/angeld29/mm8leveleditor/tree/master/src/org/gamenet/application/mm8leveleditor/data/mm6

Only retained, ignored Games.lod extracts are read. The report contains counts,
offsets, hashes and errors, never map contents, and must remain in that folder.
"""

import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path


class FormatError(ValueError):
    pass


def decode(path: Path) -> tuple[bytes, bool, int]:
    source = path.read_bytes()
    if len(source) < 14:
        raise FormatError("entry too short for size header and zlib stream")
    compressed_size, expected_size = struct.unpack_from("<II", source)
    if compressed_size != len(source) - 8 or not 0 < expected_size <= 64 * 1024 * 1024:
        raise FormatError("invalid size header")
    payload = source[8:]
    try:
        result = zlib.decompress(payload)
        checksum_valid = True
        trailing_bytes = 0
    except zlib.error as exc:
        if "incorrect data check" not in str(exc):
            raise FormatError(f"zlib stream failed: {exc}") from exc
        # Some shipped MM6 maps have an invalid Adler-32 trailer. This probe
        # accepts the raw stream only when its framing and size still check out.
        if len(payload) < 6 or payload[0] & 0x0F != 8 or (payload[0] << 8 | payload[1]) % 31:
            raise FormatError("invalid zlib header") from exc
        inflater = zlib.decompressobj(-15)
        result = inflater.decompress(payload[2:-4], expected_size + 1)
        result += inflater.flush()
        if not inflater.eof or inflater.unconsumed_tail:
            raise FormatError("raw DEFLATE stream did not reach its end") from exc
        # Record bytes after the completed DEFLATE stream. The two observed
        # MM6 entries have substantial trailers before their final four bytes;
        # a strict zlib decoder treats those bytes as an invalid checksum.
        trailing_bytes = len(inflater.unused_data)
        checksum_valid = False
    if len(result) != expected_size:
        raise FormatError(f"decoded size {len(result)} differs from header {expected_size}")
    return result, checksum_valid, trailing_bytes


class Cursor:
    def __init__(self, data: bytes):
        self.data = data
        self.offset = 0

    def take(self, size: int) -> None:
        if size < 0 or size > len(self.data) - self.offset:
            raise FormatError(f"section at {self.offset} overruns {len(self.data)} bytes")
        self.offset += size

    def count(self, name: str, record_size: int, result: dict) -> None:
        if len(self.data) - self.offset < 4:
            raise FormatError(f"missing {name} count at {self.offset}")
        count = struct.unpack_from("<I", self.data, self.offset)[0]
        self.take(4)
        if record_size and count > (len(self.data) - self.offset) // record_size:
            raise FormatError(f"{name} count {count} overruns the file")
        result[name] = count
        self.take(count * record_size)


def probe(path: Path) -> dict:
    entry: dict = {"name": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
    try:
        data, checksum_valid, trailing_bytes = decode(path)
        entry["decoded_sha256"] = hashlib.sha256(data).hexdigest()
        entry["checksum_valid"] = checksum_valid
        entry["trailing_payload_bytes"] = trailing_bytes
        entry["decoded_bytes"] = len(data)
        if len(data) < 136:
            raise FormatError("indoor header is shorter than 136 bytes")

        # Four size fields at 104..119 describe the variable arrays below.
        variable_facets, variable_rooms, variable_lights, variable_doors = struct.unpack_from("<4I", data, 104)
        entry["variable_bytes"] = {
            "facets": variable_facets, "rooms": variable_rooms,
            "room_lights": variable_lights, "doors": variable_doors,
        }
        cur = Cursor(data)
        cur.take(136)
        cur.count("vertices", 6, entry)
        cur.count("faces", 80, entry)
        cur.take(variable_facets)
        cur.take(entry["faces"] * 10)  # face bitmap names
        cur.count("face_extras", 36, entry)
        cur.take(entry["face_extras"] * 10)  # extra face bitmap names
        cur.count("rooms", 116, entry)
        cur.take(variable_rooms)
        cur.take(variable_lights)
        cur.count("doors", 0, entry)  # dynamic door records live in the DLV
        cur.count("sprites", 60, entry)  # 28-byte sprite + 32-byte name
        cur.count("lights", 12, entry)
        cur.count("bsp_nodes", 8, entry)
        cur.count("spawn_points", 20, entry)
        cur.count("outline_lines", 12, entry)
        entry["parsed_bytes"] = cur.offset
        if cur.offset != len(data):
            raise FormatError(f"parsed {cur.offset} of {len(data)} decoded bytes")
    except FormatError as exc:
        entry["error"] = str(exc)
    return entry


def probe_delta(path: Path, base: dict) -> dict:
    entry: dict = {"name": path.name}
    try:
        if "error" in base:
            raise FormatError("base map did not pass layout check")
        if not path.is_file():
            raise FormatError("delta companion is missing")
        entry["sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
        data, checksum_valid, trailing_bytes = decode(path)
        entry["decoded_sha256"] = hashlib.sha256(data).hexdigest()
        entry["decoded_bytes"] = len(data)
        entry["checksum_valid"] = checksum_valid
        entry["trailing_payload_bytes"] = trailing_bytes

        cur = Cursor(data)
        cur.take(8)  # reset count and last reset day
        cur.take(875)  # revealed map outlines
        # MM6 has no per-face or per-sprite attribute arrays here.
        cur.count("creatures", 548, entry)
        cur.count("items", 100, entry)
        # Four-byte chest header, 140 x 28-byte contents, 140 short grid
        # cells: 4 + 3920 + 280 = 4204. The MM6 dchest table says 14x10.
        cur.count("chests", 4204, entry)
        if base["doors"] != 200:
            raise FormatError(f"expected 200 base door slots, found {base['doors']}")
        door_variable_bytes = 0
        for slot in range(200):
            record_offset = cur.offset + slot * 80
            if record_offset + 76 > len(data):
                raise FormatError("door records overrun the delta")
            vertices, faces, rooms, offsets = struct.unpack_from("<4h", data, record_offset + 68)
            if min(vertices, faces, rooms, offsets) < 0:
                raise FormatError(f"door slot {slot} has a negative variable-array count")
            door_variable_bytes += 2 * (vertices + 3 * faces + rooms + 3 * offsets)
        entry["door_variable_bytes_computed"] = door_variable_bytes
        if door_variable_bytes != base["variable_bytes"]["doors"]:
            raise FormatError("door variable-array counts differ from the base header")
        cur.take(200 * 80)  # fixed door records, including unused slots
        cur.take(base["variable_bytes"]["doors"])
        cur.take(200)  # map bits
        cur.take(56)  # visit time, sky, five weather fields, reserved bytes
        entry["parsed_bytes"] = cur.offset
        if cur.offset != len(data):
            raise FormatError(f"parsed {cur.offset} of {len(data)} decoded bytes")
    except (FormatError, OSError) as exc:
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
    paths = sorted(games.glob("*.blv"))
    if not paths:
        parser.error("no retained indoor maps found")
    entries = [probe(path) for path in paths]
    deltas = [probe_delta(path.with_suffix(".dlv"), base) for path, base in zip(paths, entries)]
    failures = [entry["name"] for entry in entries + deltas if "error" in entry]
    result = {
        "format": 2,
        "source": "retained MM6 Games.lod indoor and delta extracts",
        "maps": entries,
        "deltas": deltas,
        "summary": {"base_maps": len(entries), "deltas": len(deltas),
                    "layout_failures": failures,
                    "checksum_failures": [e["name"] for e in entries + deltas if e.get("checksum_valid") is False]},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Probed {len(entries)} indoor maps and {len(deltas)} deltas; {len(failures)} layout failures")
    for entry in entries + deltas:
        if "error" in entry:
            print(f"  {entry['name']}: {entry['error']}")
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
