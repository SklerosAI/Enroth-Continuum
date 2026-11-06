#!/usr/bin/env python3
"""Check active MM6 monster-spawn operands in F3-matched retained EVT files."""

import argparse
import hashlib
import json
import struct
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research_dir", type=Path)
    root = parser.parse_args().research_dir.resolve(strict=True)
    if root.parent.name != "mm6" or root.parent.parent.name != "local-game-data":
        parser.error("research directory must be inside ignored local-game-data/mm6")
    gate = json.loads((root / "F3_GATE_REPORT_HASHES_2026-11-06.json").read_text())
    content_path = root / "F3_CONTENT_RECONCILIATION_2026-11-05.json"
    if sha256(content_path) != gate["reports_sha256"][content_path.name]:
        raise ValueError("F3 content report differs from gate")
    signed = {row["retained_path"]: row for row in json.loads(content_path.read_text())["files"]}
    events = json.loads((root / "event-probe.json").read_text())

    spawns = 0
    for event in events["files"]:
        if event["role"] not in ("listed_map", "global"):
            continue
        path = root / "icons" / event["name"]
        row = signed.get(path.relative_to(root).as_posix())
        if not row or not row["capture_stored_match"] or sha256(path) != row["retained_sha256"]:
            raise ValueError("EVT differs from F3 original match")
        raw = path.read_bytes()
        if hashlib.sha256(raw).hexdigest() != event["sha256"]:
            raise ValueError("EVT differs from F2 probe")
        for record in event["records"]:
            if record["opcode"] != 19:
                continue
            payload = raw[record["offset"]:record["offset"] + record["size"]]
            if len(payload) != 20:
                raise ValueError("MM6 monster spawn has unexpected frame width")
            slot, tier, count = struct.unpack_from("<BBB", payload, 5)
            if slot not in (1, 2, 3) or tier not in (1, 2, 3) or not 1 <= count <= 6:
                raise ValueError("MM6 monster spawn has unexpected slot, tier or count")
            spawns += 1
    if spawns != 272:
        raise ValueError("active monster spawn count differs from frozen evidence")
    print("F3-matched active MM6 monster spawns: 272/272 bounded slot, tier and count operands")


if __name__ == "__main__":
    main()
