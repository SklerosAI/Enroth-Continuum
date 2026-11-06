#!/usr/bin/env python3
"""Export six private MM6 timed-cure windows from F3-matched reports."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path


SPELLS = {40: ("Stone to Flesh", "stoned", "stone"),
          49: ("Remove Curse", "cursed", "curse"),
          57: ("Remove Fear", "afraid", "fear"),
          60: ("Cure Paralysis", "paralyzed", "paralysis"),
          64: ("Cure Insanity", "insane", "insanity"),
          67: ("Cure Weakness", "weak", "weakness")}
UNIT_SECONDS = {"minute": 60, "hour": 3600, "day": 86400}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def retained_seconds(text: str) -> int:
    match = re.search(r"\bless than (\d+) (minute|hour|day)s? per point of skill\b", text)
    if not match:
        raise ValueError("timed-cure retained rank clause lacks a bounded window")
    amount = int(match.group(1))
    if amount < 1 or amount > 100:
        raise ValueError("timed-cure rank multiplier is outside bounds")
    return amount * UNIT_SECONDS[match.group(2)]


def guide_seconds(text: str) -> int:
    match = re.search(r"(?:<(\d+)\*skill|\bskill)\s+(minute|hour|day)s?\b", text)
    if not match:
        raise ValueError("timed-cure reviewed guide clause lacks a bounded window")
    amount = int(match.group(1)) if match.group(1) else 1
    if amount < 1 or amount > 100:
        raise ValueError("timed-cure guide multiplier is outside bounds")
    return amount * UNIT_SECONDS[match.group(2)]


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
        raise ValueError("timed-cure reports fail the F3 row-count gate")
    spell_file = next(row for row in f3["files"] if row["name"].casefold() == "spells.txt")
    if (spell_file["retained_sha256"] != effect["input_sha256"]["spells"] or
            not spell_file["decoded_matches_retained"] or
            not spell_file["retained_matches_f2_catalog"]):
        raise ValueError("timed-cure report does not match the F3 original-file chain")

    shared_windows = None
    for spell_id, (name, retained_condition, guide_condition) in SPELLS.items():
        row = effect["spells"][spell_id - 1]
        reviewed = guide["rows"][spell_id - 1]
        if (row["spell_id"] != spell_id or reviewed["id"] != spell_id or
                row["retained"]["name"] != name or reviewed["name"] != name or
                reviewed["guide_target"] != "one party member" or
                "visually inspected" not in reviewed["rank_clause_evidence"] or
                retained_condition not in row["retained"]["description"].lower() or
                guide_condition not in reviewed["guide_normal_effect"].lower()):
            raise ValueError("timed-cure identity, target, condition or visual-review gate failed")
        windows = [retained_seconds(row["retained"][key]) for key in
                   ("normal_text", "expert_text", "master_text")]
        guide_windows = [guide_seconds(reviewed[key]) for key in
                         ("guide_normal_effect", "guide_expert_effect", "guide_master_effect")]
        if windows != guide_windows or windows != sorted(windows):
            raise ValueError("timed-cure retained and visually checked guide windows disagree")
        if shared_windows is None:
            shared_windows = windows
        elif windows != shared_windows:
            raise ValueError("timed-cure cohort does not share one rank window")
    return {"format": 1, "spell_report_sha256": digest(effect_bytes),
            "guide_report_sha256": digest(guide_bytes), "ids": list(SPELLS),
            "seconds_per_skill": shared_windows}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research", type=Path, help="ignored F3-matched MM6 research directory")
    parser.add_argument("output", type=Path, help="private MM6 profile rules/timed-cures.json")
    args = parser.parse_args()
    manifest = build_manifest(args.research)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, separators=(",", ":"))
        stream.write("\n")
    print("wrote private F3-matched MM6 timed-cure cohort rule")


if __name__ == "__main__":
    main()
