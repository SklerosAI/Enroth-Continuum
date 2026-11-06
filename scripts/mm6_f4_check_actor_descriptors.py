#!/usr/bin/env python3
"""Check the F3-matched MM6 actor IDs against the retained descriptor table.

Reads only ignored retained data and F3 reports. Prints aggregate counts, never
member names or game bytes. The C++ loader regression test covers row storage.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research_dir", type=Path, help="ignored MM6 research directory")
    args = parser.parse_args()

    root = args.research_dir
    report_name = "F3_ACTOR_ID_RECONCILIATION_2026-11-06.json"
    report_path = root / report_name
    gate = json.loads((root / "F3_GATE_REPORT_HASHES_2026-11-06.json").read_text())
    if sha256(report_path) != gate["reports_sha256"][report_name]:
        raise ValueError("actor report changed after F3 signoff")
    report = json.loads(report_path.read_text())

    descriptor_path = root / "icons/dmonlist.bin"
    descriptor = descriptor_path.read_bytes()
    if sha256(descriptor_path) != report["input_sha256"]["icons/dmonlist.bin"]:
        raise ValueError("retained descriptor table differs from F3 input")
    if len(descriptor) < 4:
        raise ValueError("truncated descriptor header")
    count = struct.unpack_from("<I", descriptor)[0]
    if count != 173 or len(descriptor) != 4 + count * 148:
        raise ValueError("descriptor row count or width differs from F3")
    if not any(descriptor[4 + 172 * 148 : 4 + 173 * 148]):
        raise ValueError("final descriptor row is empty")

    maps = report["maps"]
    if len(maps) != 67:
        raise ValueError("incomplete baseline map set")
    ids = []
    for map_row in maps:
        actors = map_row["actors"]
        if len(actors) != map_row["actor_count"] or not map_row["f3_original_member_matched"]:
            raise ValueError("baseline actor count or original identity differs")
        ids.extend(actor["monster_id_u8_offset_52"] for actor in actors)
    if len(ids) != 342 or any(not 1 <= actor_id <= count for actor_id in ids):
        raise ValueError("baseline actor ID outside MM6 descriptor table")
    if ids.count(173) != 1 or len(report["candidate_173"]) != 1:
        raise ValueError("baseline ID 173 reference count differs from F3")

    print("MM6 actor fixture passed: 67 map deltas, 342 actors, 173 descriptor rows, one ID 173 reference")


if __name__ == "__main__":
    main()
