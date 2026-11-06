#!/usr/bin/env python3
"""Check MM6 item EVT operands against F3-matched retained scripts and table."""

import argparse
import csv
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_write_topology(research: Path, frozen: dict[str, str], report: dict) -> None:
    """Retain item-write adjacency and selector evidence, without assuming event flow."""
    path = research / "opcode-semantics-probe.json"
    if frozen.get(str(path.relative_to(Path(__file__).resolve().parents[1]))) != digest(path):
        raise ValueError("frozen MM6 opcode evidence fingerprint mismatch")
    opcodes = json.loads(path.read_text())
    if opcodes.get("format") != 1 or len(opcodes["opcodes"]) != 43:
        raise ValueError("unexpected MM6 opcode report scope")
    if any(opcodes["input_sha256"].get(name) != expected for name, expected in
           report["input_sha256"]["events"].items()):
        raise ValueError("MM6 opcode and variable reports use different EVT bytes")

    frames = defaultdict(list)
    item_frames = []
    for opcode, summary in opcodes["opcodes"].items():
        for record in summary["records"]:
            frame = (int(opcode), record["step"], record["payload_hex"])
            frames[(record["file"], record["event_id"])].append(frame)
            if int(opcode) in (14, 16, 17) and record["payload_hex"].startswith("11"):
                item_frames.append((record, frame))

    indexed = {(record["file"], record["offset"], frame[0],
                int.from_bytes(bytes.fromhex(frame[2][2:10]), "little"))
               for record, frame in item_frames}
    expected = {(record["file"], record["offset"], record["opcode"], record["value"])
                for record in report["records"] if record["variable_id"] == 17}
    if indexed != expected or len(item_frames) != len(expected):
        raise ValueError("MM6 item writes differ between frozen event reports")

    preceding_selectors = {16: Counter(), 17: Counter()}
    subtracts_after_compare = 0
    for record, (opcode, step, payload) in item_frames:
        if opcode not in (16, 17):
            continue
        earlier = [frame for frame in frames[(record["file"], record["event_id"])]
                   if frame[1] < step]
        selectors = [frame for frame in earlier if frame[0] == 35]
        preceding_selectors[opcode][selectors[-1][2] if selectors else "none"] += 1
        if opcode == 17 and any(frame[0] == 14 and frame[2].startswith("11") and
                                frame[2][2:10] == payload[2:10] for frame in earlier):
            subtracts_after_compare += 1

    if (subtracts_after_compare != 156 or
            preceding_selectors[16] != {"none": 279, "03": 6, "04": 3, "06": 1, "05": 1} or
            preceding_selectors[17] != {"none": 29, "00": 17, "01": 17,
                                        "02": 17, "03": 17, "05": 59}):
        raise ValueError("MM6 item-write selector/compare topology changed")


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
    for path in (research / "event-variable-probe.json", research / "icons/items.txt"):
        if frozen.get(str(path.relative_to(repo))) != digest(path):
            raise ValueError("frozen MM6 item/event evidence fingerprint mismatch")

    report = json.loads((research / "event-variable-probe.json").read_text())
    if report.get("format") != 2 or report["summary"]["active_files"] != 68:
        raise ValueError("unexpected MM6 event-variable report scope")
    if digest(research / "icons/mapstats.txt") != report["input_sha256"]["mapstats"]:
        raise ValueError("retained active-map index fingerprint mismatch")
    hashes = report["input_sha256"]["events"]
    if len(hashes) != 68:
        raise ValueError("expected 68 active EVT fingerprints")
    for name, expected in hashes.items():
        if Path(name).name != name or digest(research / "icons" / name) != expected:
            raise ValueError("retained MM6 EVT fingerprint mismatch")

    item_ids = set()
    with (research / "icons/items.txt").open(encoding="cp1252", newline="") as stream:
        for row in csv.reader(stream, delimiter="\t"):
            if row and row[0].strip().isdigit():
                item_ids.add(int(row[0].strip()))
    if item_ids != set(range(581)):
        raise ValueError("retained MM6 item table is not the 0..580 set")

    item = [record for record in report["records"] if record["variable_id"] == 17]
    values = [record["value"] for record in item]
    operations = Counter(record["opcode"] for record in item)
    if (len(item) != 641 or operations != {14: 195, 16: 290, 17: 156} or
            len(set(values)) != 165 or min(values) != 1 or max(values) != 578 or
            len({record["file"] for record in item}) != 41 or
            any(record["value"] not in item_ids or record["file"] not in hashes for record in item)):
        raise ValueError("MM6 item EVT operand inventory changed")
    check_write_topology(research, frozen, report)
    print("MM6 item EVT operands: 641 active uses; 156 removals retain same-item compare topology")


if __name__ == "__main__":
    main()
