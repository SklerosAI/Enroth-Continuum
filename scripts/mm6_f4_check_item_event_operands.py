#!/usr/bin/env python3
"""Check MM6 item EVT operands against F3-matched retained scripts and table."""

import argparse
import csv
import hashlib
import json
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
    print("MM6 item EVT operands: 641 active uses; 195 read-only compares are bounded")


if __name__ == "__main__":
    main()
