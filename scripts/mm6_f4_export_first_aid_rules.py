#!/usr/bin/env python3
"""Export a private MM6 First Aid effect manifest from F3-matched retained reports."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path


SPELL_ID = 68


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def checked_amount(text: str) -> int:
    match = re.search(r"\b(?:Cures|Heal(?: one member)?)\s*=?\s*(\d+)\b", text)
    if not match:
        raise ValueError("First Aid rank claim has no bounded heal amount")
    amount = int(match.group(1))
    if amount < 1 or amount > 1000:
        raise ValueError("First Aid amount is outside bounds")
    return amount


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
        raise ValueError("First Aid reports fail the F3 row-count gate")
    spell_file = next(row for row in f3["files"] if row["name"].casefold() == "spells.txt")
    if (spell_file["retained_sha256"] != effect["input_sha256"]["spells"] or
            not spell_file["decoded_matches_retained"] or
            not spell_file["retained_matches_f2_catalog"]):
        raise ValueError("First Aid report does not match the F3 original-file chain")
    row = effect["spells"][SPELL_ID - 1]
    guide_row = guide["rows"][SPELL_ID - 1]
    if (row["spell_id"] != SPELL_ID or guide_row["id"] != SPELL_ID or
            row["retained"]["name"] != "First Aid" or guide_row["name"] != "First Aid" or
            guide_row["guide_target"] != "one party member" or
            "visually inspected" not in guide_row["rank_clause_evidence"]):
        raise ValueError("First Aid identity, target or visual-review gate failed")
    claims = [
        (row["retained"]["normal_text"], guide_row["guide_normal_effect"]),
        (row["retained"]["expert_text"], guide_row["guide_expert_effect"]),
        (row["retained"]["master_text"], guide_row["guide_master_effect"]),
    ]
    amounts = []
    for retained, checked_guide in claims:
        value = checked_amount(retained)
        if value != checked_amount(checked_guide):
            raise ValueError("First Aid retained and visually checked guide amounts disagree")
        amounts.append(value)
    if amounts != sorted(amounts):
        raise ValueError("First Aid amounts fall with mastery")
    return {"format": 1, "id": SPELL_ID, "spell_report_sha256": digest(effect_bytes),
            "guide_report_sha256": digest(guide_bytes), "heal": amounts}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research", type=Path, help="ignored F3-matched MM6 research directory")
    parser.add_argument("output", type=Path, help="private MM6 profile rules/first-aid.json")
    args = parser.parse_args()
    manifest = build_manifest(args.research)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, separators=(",", ":"))
        stream.write("\n")
    print("wrote private F3-matched MM6 First Aid rule")


if __name__ == "__main__":
    main()
