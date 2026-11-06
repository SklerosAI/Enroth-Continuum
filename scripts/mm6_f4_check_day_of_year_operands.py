#!/usr/bin/env python3
"""Check MM6 day-of-year EVT compares against the F3-matched retained ledger."""

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

    day = [record for record in report["records"] if record["variable_id"] == 19]
    values = [record["value"] for record in day]
    by_file = Counter(record["file"] for record in day)
    if (len(day) != 592 or len(set(values)) != 87 or min(values) != 76 or max(values) != 335 or
            sorted(by_file.values()) != [4, 196, 196, 196] or
            any(record["opcode"] != 14 or record["file"] not in hashes or
                record["value"] < 1 or record["value"] > 336 for record in day)):
        raise ValueError("MM6 day-of-year EVT inventory changed")
    print("MM6 day-of-year EVT operands: 592 active Compare uses, all within days 1..336")


if __name__ == "__main__":
    main()
