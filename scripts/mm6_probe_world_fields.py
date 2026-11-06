#!/usr/bin/env python3
"""Inspect retained MM6 delta record regions without assigning gameplay semantics.

The detailed per-map report stays below ignored local-game-data/mm6. This probe
never opens original archives or launches a runtime.
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path

from mm6_probe_indoors import FormatError, decode, probe as probe_indoor_base
from mm6_probe_outdoors import probe_base as probe_outdoor_base
from mm6_probe_world_roundtrip import split_delta


CHEST_WIDTH = 4204
CHEST_ITEMS_END = 4 + 140 * 28
EDITOR_GRID_END = CHEST_ITEMS_END + 81 * 2


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def counted_rows(section: bytes, width: int):
    if len(section) < 4:
        raise FormatError("missing record count")
    count = struct.unpack_from("<I", section, 0)[0]
    if count > (len(section) - 4) // width or len(section) != 4 + count * width:
        raise FormatError("record count does not fit section exactly")
    return [section[4 + i * width:4 + (i + 1) * width] for i in range(count)]


def count_nonzero(records: list[bytes]) -> int:
    return sum(any(record) for record in records)


def inspect(path: Path, indoor: bool) -> dict:
    base_path = path.with_suffix(".blv" if indoor else ".odm")
    base = probe_indoor_base(base_path) if indoor else probe_outdoor_base(base_path)
    if "error" in base:
        raise FormatError("paired base layout failed bounded probe")
    door_bytes = base["variable_bytes"]["doors"] if indoor else 0
    data, valid, trailing = decode(path)
    if not valid or trailing:
        raise FormatError("invalid delta framing or checksum")
    parts = dict(split_delta(data, indoor, door_bytes))
    actors = counted_rows(parts["creatures"], 548)
    objects = counted_rows(parts["ground_items"], 100)
    chests = counted_rows(parts["chests"], CHEST_WIDTH)
    if len(chests) != 20:
        raise FormatError("unexpected chest count")
    if any(len(row) != CHEST_WIDTH for row in chests):
        raise FormatError("short chest record")
    door_rows = [parts["door_records"][i:i + 80] for i in range(0, 200 * 80, 80)] if indoor else []
    return {
        "name": path.name,
        "entry_sha256": sha256(path.read_bytes()),
        "decoded_sha256": sha256(data),
        "counts": {"actors": len(actors), "objects": len(objects), "chests": len(chests), "doors": len(door_rows)},
        "nonzero_records": {
            "actors": count_nonzero(actors), "objects": count_nonzero(objects),
            "chest_header_0_3": count_nonzero([row[:4] for row in chests]),
            "chest_items_4_3923": count_nonzero([row[4:CHEST_ITEMS_END] for row in chests]),
            "chest_first_81_shorts_3924_4085": count_nonzero([row[CHEST_ITEMS_END:EDITOR_GRID_END] for row in chests]),
            "chest_last_59_shorts_4086_4203": count_nonzero([row[EDITOR_GRID_END:] for row in chests]),
            "doors": count_nonzero(door_rows),
        },
        "nonzero_sections": {name: any(parts[name]) for name in (
            "reset_header", "persistent_bytes", "tail", "door_variable_arrays") if name in parts},
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="retained ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="JSON report inside that directory")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must be beneath ignored local-game-data/mm6")
    if report.suffix != ".json" or not report.is_relative_to(root) or not report.parent.is_dir():
        parser.error("report must be a JSON file inside the ignored reference directory")
    games = root / "Games"
    paths = [(p, True) for p in sorted(games.glob("*.dlv"))] + [(p, False) for p in sorted(games.glob("*.ddm"))]
    if sum(indoor for _, indoor in paths) != 52 or len(paths) != 67:
        parser.error("expected 52 indoor and 15 outdoor retained deltas")
    entries = [inspect(path, indoor) for path, indoor in paths]
    totals = {key: sum(entry["nonzero_records"][key] for entry in entries) for key in entries[0]["nonzero_records"]}
    counts = {key: sum(entry["counts"][key] for entry in entries) for key in entries[0]["counts"]}
    summary = {
        "maps": len(entries), "counts": counts, "nonzero_records": totals,
        "nonzero_sections": {name: sum(entry["nonzero_sections"].get(name, False) for entry in entries)
                             for name in ("reset_header", "persistent_bytes", "tail", "door_variable_arrays")},
    }
    result = {
        "format": 1,
        "scope": "retained ignored MM6 delta extracts only; no original archives or runtime",
        "limits": "All-zero chest grid region cannot establish its cell geometry or gameplay meaning.",
        "summary": summary,
        "entries": entries,
        "source_sha256": {name: sha256((repo / name).read_bytes()) for name in (
            "scripts/mm6_probe_world_fields.py", "scripts/mm6_probe_world_roundtrip.py",
            "scripts/mm6_probe_indoors.py", "scripts/mm6_probe_outdoors.py")},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
