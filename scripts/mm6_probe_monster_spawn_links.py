#!/usr/bin/env python3
"""Join retained MM6 monster rows, descriptors, animation names and map actors.

The 548-byte actor's monster-ID offset is a pinned editor-source hypothesis.
This F2 probe reads only ignored retained extracts, never the original install.
Its per-ID, per-map and resource-name output must remain ignored locally.
"""

import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct

from mm6_probe_indoors import decode


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def name(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("cp1252", errors="replace").strip()


def normalized(value: str) -> str:
    return "".join(value.split()).casefold()


def monster_rows(path: Path) -> dict[int, str]:
    rows = {}
    with path.open(encoding="cp1252", errors="replace", newline="") as stream:
        for row in csv.reader(stream, delimiter="\t"):
            if row and row[0].strip().isdigit():
                number = int(row[0].strip())
                if number in rows or len(row) != 33:
                    raise ValueError(f"duplicate or malformed monster row {number}")
                rows[number] = row[1].strip().strip('"')
    if set(rows) != set(range(1, 174)):
        raise ValueError("expected retained monster IDs 1..173")
    return rows


def descriptors(path: Path) -> list[dict]:
    data = path.read_bytes()
    if len(data) < 4:
        raise ValueError("short monster descriptor table")
    count = struct.unpack_from("<I", data)[0]
    if count != 173 or len(data) != 4 + count * 148:
        raise ValueError("unexpected MM6 monster descriptor count or length")
    return [
        {
            "id": i + 1,
            "internal_name": name(data[4 + i * 148 + 16:4 + i * 148 + 48]),
            "animation_names": [name(data[4 + i * 148 + 48 + j * 10:4 + i * 148 + 58 + j * 10])
                                for j in range(8)],
        }
        for i in range(count)
    ]


def frame_index(path: Path) -> tuple[set[str], dict]:
    data = path.read_bytes()
    if len(data) < 8:
        raise ValueError("short sprite-frame table")
    frames, extended = struct.unpack_from("<II", data)
    if len(data) != 8 + frames * 56 + extended * 2:
        raise ValueError("unexpected MM6 sprite-frame length")
    indices = struct.unpack_from(f"<{extended}H", data, 8 + frames * 56)
    if any(index >= frames for index in indices):
        raise ValueError("extended sprite index exceeds frame count")
    names = [name(data[8 + index * 56:8 + index * 56 + 12]).casefold() for index in indices]
    return set(names), {"frames": frames, "extended_indices": extended,
                        "extended_names_sorted": names == sorted(names)}


def map_actors(folder: Path) -> tuple[list[dict], dict[int, int]]:
    paths = sorted([*folder.glob("*.dlv"), *folder.glob("*.ddm")])
    if len(paths) != 67:
        raise ValueError(f"expected 67 retained map deltas, found {len(paths)}")
    result = []
    by_id: dict[int, int] = {}
    for path in paths:
        data, checksum_valid, trailer = decode(path)
        if not checksum_valid or trailer:
            raise ValueError(f"unexpected delta compression anomaly: {path.name}")
        offset = 8 + (875 if path.suffix == ".dlv" else 1936)
        if offset + 4 > len(data):
            raise ValueError(f"short creature count: {path.name}")
        count = struct.unpack_from("<I", data, offset)[0]
        offset += 4
        if count > (len(data) - offset) // 548:
            raise ValueError(f"creature rows overrun delta: {path.name}")
        ids = []
        for index in range(count):
            number = data[offset + index * 548 + 52]
            if not 1 <= number <= 173:
                raise ValueError(f"out-of-range creature ID candidate: {path.name}")
            ids.append(number)
            by_id[number] = by_id.get(number, 0) + 1
        result.append({"map_delta": path.name, "entry_sha256": sha256(path), "actor_ids": ids})
    return result, by_id


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    output = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must stay in ignored local-game-data/mm6")
    if not output.is_relative_to(root) or output.suffix.lower() != ".json" or not output.parent.is_dir():
        parser.error("report must be JSON inside the ignored reference directory")

    source = repo / "src/Engine/Snapshots/TableSerialization.cpp"
    if "monsters.pop_back(); // The final entry is unused in MM6 too." not in source.read_text():
        parser.error("monster descriptor load path changed; review probe")
    table = monster_rows(root / "icons/monsters.txt")
    desc = descriptors(root / "icons/dmonlist.bin")
    frame_names, frame_summary = frame_index(root / "icons/dsft.bin")
    maps, placed = map_actors(root / "Games")
    for record in desc:
        number = record["id"]
        record["name_matches_table"] = normalized(record["internal_name"]) == normalized(table[number])
        record["animations"] = [
            {"name": value, "frame_index_match": value.casefold() in frame_names}
            for value in record.pop("animation_names") if value
        ]
        record["baseline_actor_count"] = placed.get(number, 0)

    animation_count = sum(len(record["animations"]) for record in desc)
    matched = sum(a["frame_index_match"] for record in desc for a in record["animations"])
    summary = {
        "monster_rows": len(table), "descriptors": len(desc),
        "normalized_descriptor_name_matches": sum(record["name_matches_table"] for record in desc),
        "animation_references": animation_count, "animation_frame_name_matches": matched,
        "animation_frame_name_misses": animation_count - matched,
        **frame_summary,
        "retained_map_deltas": len(maps), "baseline_actors": sum(placed.values()),
        "distinct_baseline_monster_ids": len(placed),
        "baseline_actors_with_id_173": placed.get(173, 0),
        "descriptors_loaded_by_current_mm6_path": 172,
    }
    payload = {
        "format": 1,
        "scope": "retained MM6 extracts plus current source; no original install or MM6 launch",
        "actor_id_offset_confidence": "pinned MM8LevelEditor MM6 Creature.java offset 52; original behavior unconfirmed",
        "summary": summary, "descriptors": desc, "map_actors": maps,
        "sha256": {
            "icons/monsters.txt": sha256(root / "icons/monsters.txt"),
            "icons/dmonlist.bin": sha256(root / "icons/dmonlist.bin"),
            "icons/dsft.bin": sha256(root / "icons/dsft.bin"),
            "src/Engine/Snapshots/TableSerialization.cpp": sha256(source),
            "src/Engine/Objects/Actor.cpp": sha256(repo / "src/Engine/Objects/Actor.cpp"),
            "src/Engine/Graphics/Sprites.cpp": sha256(repo / "src/Engine/Graphics/Sprites.cpp"),
        },
    }
    output.write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    main()
