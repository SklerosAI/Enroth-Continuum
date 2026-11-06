#!/usr/bin/env python3
"""Export private MM6 Shield/Heroism buff rules from F3-matched reports."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path


SPELLS = {17: "Shield", 51: "Heroism"}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def retained_duration(text: str) -> tuple[int, int]:
    match = re.search(r"Duration (\d+) hour\s*\+\s*(\d+) minutes", text)
    if not match:
        raise ValueError("combat buff retained rank lacks a bounded duration")
    return int(match.group(1)) * 3600, int(match.group(2)) * 60


def guide_duration(text: str) -> tuple[int, int]:
    match = re.search(r"(\d+) hour\s*\+\s*(\d+)\*skill minutes", text)
    if not match:
        raise ValueError("combat buff reviewed guide rank lacks a bounded duration")
    return int(match.group(1)) * 3600, int(match.group(2)) * 60


def heroism_power(retained: str, reviewed: str) -> tuple[int, int]:
    source = re.search(r"\bby (\d+)\s*\+\s*(\d+) point per point of skill", retained)
    guide = re.search(r"Damage bonus (\d+)\s*\+\s*skill", reviewed)
    if not source or not guide:
        raise ValueError("Heroism effect claims lack a bounded power formula")
    source_terms = int(source.group(1)), int(source.group(2))
    if source_terms != (int(guide.group(1)), 1):
        raise ValueError("Heroism retained and reviewed guide power disagree")
    return source_terms


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
        raise ValueError("combat buff reports fail the F3 row-count gate")
    spell_file = next(row for row in f3["files"] if row["name"].casefold() == "spells.txt")
    if (spell_file["retained_sha256"] != effect["input_sha256"]["spells"] or
            not spell_file["decoded_matches_retained"] or
            not spell_file["retained_matches_f2_catalog"]):
        raise ValueError("combat buff report does not match the F3 original-file chain")

    shared_duration = None
    power = None
    for spell_id, name in SPELLS.items():
        row = effect["spells"][spell_id - 1]
        reviewed = guide["rows"][spell_id - 1]
        retained = row["retained"]
        if (row["spell_id"] != spell_id or reviewed["id"] != spell_id or
                retained["name"] != name or reviewed["name"] != name or
                reviewed["guide_target"] != "one party member" or
                "visually inspected" not in reviewed["rank_clause_evidence"] or
                retained["expert_text"] != "Spell affects entire party" or
                "party" not in reviewed["guide_expert_effect"].lower()):
            raise ValueError("combat buff identity, target or visual-review gate failed")
        normal = retained_duration(retained["normal_text"])
        master = retained_duration(retained["master_text"])
        if (normal != guide_duration(reviewed["guide_normal_effect"]) or
                master != guide_duration(reviewed["guide_master_effect"])):
            raise ValueError("combat buff retained and reviewed guide durations disagree")
        if normal[0] != master[0] or any(value < 1 or value > 86400 for value in normal + master):
            raise ValueError("combat buff duration is outside the shared bounded shape")
        duration = {"base_seconds": normal[0],
                    "per_skill_seconds": [normal[1], normal[1], master[1]]}
        if shared_duration is None:
            shared_duration = duration
        elif shared_duration != duration:
            raise ValueError("Shield/Heroism do not share rank duration")
        if spell_id == 17:
            if ("Halves damage from incoming ranged attacks" not in retained["description"] or
                    "Halve missile damage" not in reviewed["guide_normal_effect"]):
                raise ValueError("Shield missile-damage effect claim is not reviewed")
        else:
            power = heroism_power(retained["description"], reviewed["guide_normal_effect"])
    return {"format": 1, "spell_report_sha256": digest(effect_bytes),
            "guide_report_sha256": digest(guide_bytes), "ids": list(SPELLS),
            **shared_duration, "heroism_power": list(power)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research", type=Path, help="ignored F3-matched MM6 research directory")
    parser.add_argument("output", type=Path, help="private MM6 profile rules/combat-buffs.json")
    args = parser.parse_args()
    manifest = build_manifest(args.research)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, separators=(",", ":"))
        stream.write("\n")
    print("wrote private F3-matched MM6 combat buff rules")


if __name__ == "__main__":
    main()
