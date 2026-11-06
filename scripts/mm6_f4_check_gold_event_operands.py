#!/usr/bin/env python3
"""Check MM6 fixed-gold EVT operands against the F3-matched retained ledger."""

import argparse
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

    report = json.loads((research / "event-variable-probe.json").read_text())
    if report.get("format") != 2 or report["summary"]["active_files"] != 68:
        raise ValueError("unexpected MM6 event-variable report version or scope")
    if digest(research / "icons" / "mapstats.txt") != report["input_sha256"]["mapstats"]:
        raise ValueError("retained active-map index fingerprint mismatch")
    hashes = report["input_sha256"]["events"]
    if len(hashes) != 68:
        raise ValueError("expected 68 retained active EVT fingerprints")
    for name, expected in hashes.items():
        if Path(name).name != name or digest(research / "icons" / name) != expected:
            raise ValueError("retained EVT fingerprint mismatch")

    gold = [record for record in report["records"] if record["variable_id"] == 21]
    operations = Counter(record["opcode"] for record in gold)
    values = [record["value"] for record in gold]
    if (len(gold) != 147 or operations != {14: 14, 16: 111, 17: 22} or
            min(values) != 1 or max(values) != 250000 or
            len({record["file"] for record in gold}) != 12 or
            any(record["value"] < 0 or record["file"] not in hashes for record in gold)):
        raise ValueError("MM6 fixed-gold EVT inventory changed")
    print("MM6 fixed-gold EVT operands: 147 active uses, all nonnegative and bounded")


if __name__ == "__main__":
    main()
