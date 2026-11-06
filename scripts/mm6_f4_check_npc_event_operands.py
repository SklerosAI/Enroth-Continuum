#!/usr/bin/env python3
"""Check MM6 NPC topic/move EVT operands against frozen retained scripts."""

import argparse
import csv
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research", type=Path)
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    research = args.research.resolve(strict=True)
    if research == allowed or not research.is_relative_to(allowed):
        parser.error("research must be an ignored MM6 reference directory")

    frozen = json.loads((research / "F2_FREEZE_MANIFEST.json").read_text())["ignored_derived_sha256"]
    for path in (research / "event-probe.json", research / "campaign-routes-probe.json",
                 research / "icons/mapstats.txt", research / "icons/2devents.txt"):
        if frozen.get(str(path.relative_to(repo))) != digest(path):
            raise ValueError("frozen MM6 NPC evidence fingerprint mismatch")

    report = json.loads((research / "event-probe.json").read_text())
    if report.get("format") != 1 or report["summary"]["active_records"] != 14477:
        raise ValueError("unexpected MM6 active EVT report scope")
    houses = set()
    with (research / "icons/2devents.txt").open(encoding="cp1252", newline="") as stream:
        for row in csv.reader(stream, delimiter="\t"):
            if row and row[0].strip().isdigit():
                houses.add(int(row[0].strip()))
    if houses != set(range(1, 558)):
        raise ValueError("retained MM6 house table changed")

    topics = []
    moves = []
    active_files = 0
    for entry in report["files"]:
        if entry["role"] not in ("listed_map", "global"):
            continue
        active_files += 1
        name = entry["name"]
        if Path(name).name != name:
            raise ValueError("invalid retained EVT name")
        data = (research / "icons" / name).read_bytes()
        if digest(research / "icons" / name) != entry["sha256"]:
            raise ValueError("retained MM6 EVT fingerprint mismatch")
        for record in entry["records"]:
            if record["opcode"] not in (39, 40):
                continue
            offset, size = record["offset"], record["size"]
            if offset < 0 or size < 5 or offset + size > len(data) or data[offset] + 1 != size:
                raise ValueError("malformed retained MM6 NPC event frame")
            payload = data[offset + 5:offset + size]
            if record["opcode"] == 39:
                if len(payload) != 9:
                    raise ValueError("invalid MM6 SetNPCTopic payload")
                topics.append(struct.unpack("<IBI", payload))
            else:
                if len(payload) != 8:
                    raise ValueError("invalid MM6 MoveNPC payload")
                moves.append(struct.unpack("<II", payload))

    routes = json.loads((research / "campaign-routes-probe.json").read_text())["summary"]
    if (active_files != 68 or len(topics) != 132 or len(moves) != 29 or
            Counter(slot for _, slot, _ in topics) != {0: 82, 1: 38, 2: 12} or
            min(npc for npc, _, _ in topics) != 1 or max(npc for npc, _, _ in topics) != 330 or
            min(event for _, _, event in topics) != 0 or max(event for _, _, event in topics) != 364 or
            min(npc for npc, _ in moves) != 12 or max(npc for npc, _ in moves) != 330 or
            min(house for _, house in moves) != 0 or max(house for _, house in moves) != 486 or
            any(house not in houses and house != 0 for _, house in moves) or
            routes["npc_teacher_range_with_global_references"] != 74 or
            routes["npc_teacher_range_with_global_distinct_ids"] != 43 or
            routes["npc_generic_missing_global_references"] != 35):
        raise ValueError("retained MM6 NPC event inventory changed")
    print("MM6 NPC events: 132 topic writes, 29 moves, 74 script topics in inherited teacher range")


if __name__ == "__main__":
    main()
