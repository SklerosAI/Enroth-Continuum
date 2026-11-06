#!/usr/bin/env python3
"""Check MM6 award EVT operands against the F3-matched retained event ledger."""

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

    award = [record for record in report["records"] if record["variable_id"] == 12]
    opcodes = Counter(record["opcode"] for record in award)
    values = [record["value"] for record in award]
    if (len(award) != 193 or opcodes != {14: 63, 16: 125, 18: 5} or
            min(values) != 2 or max(values) != 61 or
            any(value < 1 or value > 100 for value in values) or
            any(record["file"] not in hashes for record in award)):
        raise ValueError("MM6 award EVT operand inventory changed")
    print("MM6 award EVT operands: 193 active uses, all within retained award IDs 1..100")


if __name__ == "__main__":
    main()
