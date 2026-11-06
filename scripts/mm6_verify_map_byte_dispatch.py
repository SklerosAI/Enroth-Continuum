#!/usr/bin/env python3
"""Check the F3-matched retained EVT probe against the bounded F4 MapVar path."""

import argparse
import json
from collections import Counter
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path, help="Ignored event-variable-probe.json")
    args = parser.parse_args()

    report = json.loads(args.report.read_text())
    assert report["format"] == 2
    records = [r for r in report["records"] if 105 <= r["variable_id"] <= 204]
    assert len(records) == 1240, "F3-matched map-variable corpus changed"
    assert all(105 <= r["variable_id"] <= 175 for r in records)
    assert all(r["opcode"] in (14, 16, 17, 18) for r in records)
    assert all(0 <= r["value"] <= 255 for r in records)
    counts = Counter(r["opcode"] for r in records)
    assert counts == {14: 581, 16: 131, 17: 22, 18: 506}
    print("MM6 MapVar dispatch coverage: 1240 retained operations within active IDs and byte operands")


if __name__ == "__main__":
    main()
