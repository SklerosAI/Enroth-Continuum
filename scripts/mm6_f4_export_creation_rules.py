#!/usr/bin/env python3
"""Export the private F3 creation arrays to a local MM6 profile manifest."""

import argparse
import hashlib
import json
import os
from pathlib import Path


def checked_vector(value, count, low, high, label):
    if not isinstance(value, list) or len(value) != count:
        raise ValueError(f"{label}: expected {count} entries")
    if any(type(item) is not int or item < low or item > high for item in value):
        raise ValueError(f"{label}: invalid entry")
    return value


def build_manifest(report_bytes):
    report = json.loads(report_bytes)
    if report.get("status") != "corrected_authoritative_static_analysis":
        raise ValueError("expected authoritative corrected F3 PE report")
    creation = report["creation"]
    arrays = creation["arrays"]
    fields = {
        "hp_base": checked_vector(arrays["hp_base"], 6, 0, 255, "hp_base"),
        "sp_base": checked_vector(arrays["sp_base"], 6, 0, 255, "sp_base"),
        "hp_factor": checked_vector(arrays["hp_factor"], 18, 0, 255, "hp_factor"),
        "sp_factor": checked_vector(arrays["sp_factor"], 18, 0, 255, "sp_factor"),
    }
    stats = creation["starting_stats_by_family"]
    skills = creation["starting_skills_by_family"]
    if not isinstance(stats, list) or len(stats) != 6 or not isinstance(skills, list) or len(skills) != 6:
        raise ValueError("expected six creation families")
    for family, (stat_row, skill_row) in enumerate(zip(stats, skills)):
        checked_vector(stat_row, 7, 0, 255, f"stats family {family}")
        checked_vector(skill_row, 31, 0, 3, f"skills family {family}")
        if sum(stat_row) != 73 or skill_row.count(1) != 2 or skill_row.count(2) != 9:
            raise ValueError(f"family {family}: invalid creation shape")
    return {
        "format": 1,
        "source_report_sha256": hashlib.sha256(report_bytes).hexdigest(),
        **fields,
        "starting_stats": stats,
        "starting_skills": skills,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, required=True, help="ignored corrected F3 PE v2 JSON")
    parser.add_argument("--output", type=Path, required=True, help="private MM6 profile rules/creation.json")
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit("refusing to overwrite an existing private rules manifest")
    manifest = build_manifest(args.report.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, separators=(",", ":"))
        stream.write("\n")
    print("wrote private MM6 creation rules manifest")


if __name__ == "__main__":
    main()
