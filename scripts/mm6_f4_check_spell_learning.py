#!/usr/bin/env python3
"""Verify the private MM6 class/school matrix and record the rank-data gap."""

import argparse
import hashlib
import json
from pathlib import Path


def check(research_dir: Path, creation_manifest: Path):
    report_path = research_dir / "F3_PE_RECONCILIATION_2026-11-05_v2.json"
    table_path = research_dir / "F3_PE_TABLES_2026-11-05.json"
    report_bytes = report_path.read_bytes()
    table_bytes = table_path.read_bytes()
    report = json.loads(report_bytes)
    tables = json.loads(table_bytes)
    manifest = json.loads(creation_manifest.read_bytes())

    assert report["status"] == "corrected_authoritative_static_analysis"
    assert report["original_exe_sha256"] == tables["exe_sha256"]
    assert report["input_sha256"][table_path.name] == hashlib.sha256(table_bytes).hexdigest()
    assert manifest["source_report_sha256"] == hashlib.sha256(report_bytes).hexdigest()
    skills = report["creation"]["starting_skills_by_family"]
    assert manifest["starting_skills"] == skills
    assert len(skills) == 6
    assert all(len(row) == 31 and all(type(code) is int and 0 <= code <= 3 for code in row)
               for row in skills)

    spell_rows = tables["tables"]["spells"]["rows"]
    assert len(spell_rows) == 100
    assert [row["id"] for row in spell_rows] == list(range(100))
    assert len({row["bits"] for row in spell_rows[1:]}) == 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research_dir", type=Path, help="Ignored F3 research directory")
    parser.add_argument("creation_manifest", type=Path, help="Private MM6 rules/creation.json")
    args = parser.parse_args()
    check(args.research_dir, args.creation_manifest)
    print("Verified six MM6 class/school rows against the private F3 profile; no per-spell mastery field in the captured spell records.")
