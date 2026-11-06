#!/usr/bin/env python3
"""Write a private per-ID MM6 effect-dispatch handoff from retained reports."""

import argparse
import hashlib
import json
import re
from pathlib import Path


SOURCES = {
    "queue": "src/Engine/Spells/CastSpellInfo.cpp",
    "event": "src/Engine/Spells/Spells.cpp",
    "monster": "src/Engine/Objects/Actor.cpp",
}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def case_lines(source: str, enum_name: str) -> str:
    pattern = re.compile(r"\bcase\s+" + re.escape(enum_name) + r"\s*:")
    found = [str(number) for number, line in enumerate(source.splitlines(), start=1) if pattern.search(line)]
    return ",".join(found) or "—"


def escape(value: object) -> str:
    return str(value).replace("|", "\\|").replace("\n", " ")


def build(research_dir: Path, source_root: Path) -> str:
    effect_path = research_dir / "spell-effect-probe.json"
    guide_path = research_dir / "spell-guide-claims-matrix-private.json"
    f3_path = research_dir / "F3_CONTENT_RECONCILIATION_2026-11-05.json"
    effect = json.loads(effect_path.read_bytes())
    guide = json.loads(guide_path.read_bytes())
    f3 = json.loads(f3_path.read_bytes())
    assert len(effect["spells"]) == len(guide["rows"]) == 99
    assert not f3["problems"]
    by_name = {row["name"].casefold(): row for row in f3["files"]}
    for name, key in (("items.txt", "items"), ("spells.txt", "spells")):
        row = by_name[name]
        assert row["retained_sha256"] == effect["input_sha256"][key]
        assert row["decoded_matches_retained"] and row["retained_matches_f2_catalog"]

    source_text = {name: (source_root / path).read_text() for name, path in SOURCES.items()}
    lines = [
        "# MM6 F4 spell effect dispatch — private per-ID handoff",
        "",
        "Retained item/spell reports are tied to F3 original-matched table hashes. Guide and C++ source comparisons are hypotheses, not observed effects. The present MM6 runtime blocks all 99 regular spell effects before inherited player/event/monster/damage dispatch.",
        "",
        "## Source fingerprints",
        "",
        f"- Retained effect probe SHA-256: `{digest(effect_path)}`",
        f"- Private guide matrix SHA-256: `{digest(guide_path)}`",
        f"- F3 content reconciliation SHA-256: `{digest(f3_path)}`",
    ]
    for name, path in SOURCES.items():
        lines.append(f"- Current {name} source SHA-256: `{digest(source_root / path)}`")
    lines += [
        "",
        "## Coverage and proof limits",
        "",
        "- All 99 IDs have a current numeric player path: 97 in the inherited switch and two direct portal/beacon branches. Presence is not semantic proof.",
        "- Every row below is blocked for MM6 until the title-specific target, Normal/Expert/Master effects, damage/resistance, recovery/failure timing, visual/audio and persistence are reconciled. Event and monster use need their own source-specific checks.",
        "- `queue`, `event`, and `monster` columns list current `case` labels in those files. Multiple labels or no label do not establish whether a branch is reachable. Use the report hashes above to re-audit after source changes.",
        "",
        "## Per-ID register",
        "",
        "| ID | Retained name | Inherited numeric label | Label join | Guide assessment | Guide page/target | Queue case lines | Event case lines | Monster case lines | Active EVT / monster refs | Gate |",
        "| ---: | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for spell_id, (row, guide_row) in enumerate(zip(effect["spells"], guide["rows"]), start=1):
        assert row["spell_id"] == guide_row["id"] == spell_id
        enum_name = row["current_enum_name"]
        refs = f"{len(row['active_event_casts'])}/{len(row['monster_spell_label_uses'])}"
        guide_loc = f"{guide_row['guide_printed_page']} / {guide_row['guide_target']}"
        fields = [spell_id, row["retained"]["name"], enum_name, row["label_status"],
                  guide_row["same_id_cpp_assessment"], guide_loc,
                  case_lines(source_text["queue"], enum_name),
                  case_lines(source_text["event"], enum_name),
                  case_lines(source_text["monster"], enum_name), refs, "blocked"]
        lines.append("| " + " | ".join(escape(field) for field in fields) + " |")
    lines += [
        "",
        "## Next evidence for each ID",
        "",
        "1. Reconcile retained spell and guide rank clauses with the corrected PE cost/recovery row and a title-specific target/effect specification. A same-name inherited label is insufficient.",
        "2. Trace the MM6 original executable routine or an independently corroborated original-behavior source for target, damage, failure and persistence rules; record unresolved disputes by ID here.",
        "3. Add offline Normal/Expert/Master state tests before allowing that ID through the MM6 guard; then inspect graphical/audio output and save/reload in F5. Keep scroll, event and monster paths separate.",
        "",
    ]
    return "\n".join(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research_dir", type=Path, help="Ignored MM6 F3-matched research directory")
    parser.add_argument("source_root", type=Path, help="Current Enroth source checkout")
    parser.add_argument("output", type=Path, help="New ignored private-wiki Markdown page")
    args = parser.parse_args()
    page = build(args.research_dir, args.source_root)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as stream:
        stream.write(page)
    print("Wrote private 99-ID MM6 effect dispatch handoff without publishing rows.")
