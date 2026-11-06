#!/usr/bin/env python3
"""Export F3-matched MM6 spell costs and delays to a private profile manifest."""

import argparse
import hashlib
import json
import os
from pathlib import Path


def checked_triple(value, low, high, label):
    if not isinstance(value, list) or len(value) != 3:
        raise ValueError(f"{label}: expected Normal/Expert/Master triple")
    if any(type(number) is not int or number < low or number > high for number in value):
        raise ValueError(f"{label}: invalid value")
    return value


def build_manifest(report_bytes, gate):
    report_hash = hashlib.sha256(report_bytes).hexdigest()
    if gate["reports_sha256"]["F3_PE_RECONCILIATION_2026-11-05_v2.json"] != report_hash:
        raise ValueError("corrected PE report does not match the accepted F3 gate")
    report = json.loads(report_bytes)
    if report.get("status") != "corrected_authoritative_static_analysis" or report.get("format") != 1:
        raise ValueError("expected corrected F3 PE v2 report")
    rows = report["spell_recovery"]["rows"]
    if not isinstance(rows, list) or len(rows) != 99:
        raise ValueError("expected exactly 99 MM6 spell rows")
    if sum(not row["spell_points_match"] for row in rows) != 1:
        raise ValueError("expected one executable/text cost disagreement")

    spells = []
    for spell_id, row in enumerate(rows, 1):
        if type(row["id"]) is not int or row["id"] != spell_id:
            raise ValueError("MM6 spell rows are not in exact ID order")
        mana = checked_triple(row["file_spell_points"], 0, 32767, f"mana {spell_id}")
        recovery = checked_triple(row["file_delay"], 1, 32767, f"recovery {spell_id}")
        if recovery != sorted(recovery, reverse=True):
            raise ValueError("MM6 recovery does not decrease with mastery")
        spells.append({"id": spell_id, "mana": mana, "recovery": recovery})
    return {"format": 1, "source_report_sha256": report_hash, "spells": spells}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, required=True, help="ignored corrected F3 PE v2 JSON")
    parser.add_argument("--gate", type=Path, required=True, help="ignored F3 gate report hash registry")
    parser.add_argument("--output", type=Path, required=True, help="private MM6 profile rules/spells.json")
    args = parser.parse_args()
    manifest = build_manifest(args.report.read_bytes(), json.loads(args.gate.read_text()))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, separators=(",", ":"))
        stream.write("\n")
    print("wrote private MM6 spell rules manifest")


if __name__ == "__main__":
    main()
