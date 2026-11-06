#!/usr/bin/env python3
"""Export private MM6 Power Cure effect coefficients from F3-matched reports."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path


SPELL_ID = 77


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def coefficients(text: str) -> tuple[int, int]:
    numbers = [int(value) for value in re.findall(r"\b\d+\b", text)]
    if len(numbers) != 2 or any(value < 1 or value > 100 for value in numbers):
        raise ValueError("Power Cure effect claim lacks bounded base and skill terms")
    return numbers[0], numbers[1]


def build_manifest(research: Path) -> dict:
    effect_path = research / "spell-effect-probe.json"
    guide_path = research / "spell-guide-claims-matrix-private.json"
    f3_path = research / "F3_CONTENT_RECONCILIATION_2026-11-05.json"
    effect_bytes = effect_path.read_bytes()
    guide_bytes = guide_path.read_bytes()
    effect = json.loads(effect_bytes)
    guide = json.loads(guide_bytes)
    f3 = json.loads(f3_path.read_bytes())
    if f3["problems"] or len(effect["spells"]) != 99 or len(guide["rows"]) != 99:
        raise ValueError("Power Cure reports fail the F3 row-count gate")
    spell_file = next(row for row in f3["files"] if row["name"].casefold() == "spells.txt")
    if (spell_file["retained_sha256"] != effect["input_sha256"]["spells"] or
            not spell_file["decoded_matches_retained"] or
            not spell_file["retained_matches_f2_catalog"]):
        raise ValueError("Power Cure report does not match the F3 original-file chain")
    row = effect["spells"][SPELL_ID - 1]
    guide_row = guide["rows"][SPELL_ID - 1]
    if (row["spell_id"] != SPELL_ID or guide_row["id"] != SPELL_ID or
            row["retained"]["name"] != "Power Cure" or guide_row["name"] != "Power Cure" or
            guide_row["guide_target"] != "party" or
            "visually inspected" not in guide_row["rank_clause_evidence"]):
        raise ValueError("Power Cure identity, target or visual-review gate failed")
    retained_terms = coefficients(row["retained"]["description"])
    guide_terms = coefficients(guide_row["guide_normal_effect"])
    if retained_terms != guide_terms:
        raise ValueError("Power Cure retained and visually checked guide formulas disagree")
    return {"format": 1, "id": SPELL_ID, "spell_report_sha256": digest(effect_bytes),
            "guide_report_sha256": digest(guide_bytes), "base": retained_terms[0],
            "per_skill": retained_terms[1]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research", type=Path, help="ignored F3-matched MM6 research directory")
    parser.add_argument("output", type=Path, help="private MM6 profile rules/power-cure.json")
    args = parser.parse_args()
    manifest = build_manifest(args.research)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, separators=(",", ":"))
        stream.write("\n")
    print("wrote private F3-matched MM6 Power Cure rule")


if __name__ == "__main__":
    main()
