#!/usr/bin/env python3
"""Audit all retained MM6 spell rows against current effect and item dispatch.

This is an F2 static/source join. Exact spell text and per-ID evidence are
written only to an ignored local report; output does not prove spell effects.
"""

import argparse
import csv
import hashlib
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

from mm6_probe_spells import enum_spell_ids, mm6_spell_rows, normalized_name


SOURCE_NAMES = (
    "scripts/mm6_probe_spell_effects.py",
    "scripts/mm6_probe_spells.py",
    "src/Engine/Spells/SpellEnums.h",
    "src/Engine/Spells/SpellEnumFunctions.h",
    "src/Engine/Spells/Spells.cpp",
    "src/Engine/Spells/CastSpellInfo.cpp",
    "src/Engine/Objects/ItemEnums.h",
    "src/Engine/Objects/ItemEnumFunctions.cpp",
    "src/Engine/Objects/ItemEnumFunctions.h",
    "src/Engine/Objects/Item.h",
    "src/Engine/Objects/Character.cpp",
    "src/Engine/Objects/Actor.cpp",
    "src/Engine/Objects/Monsters.cpp",
    "src/Engine/Evt/EvtInstruction.cpp",
    "src/Engine/Evt/EvtInterpreter.cpp",
    "src/GUI/UI/UISpellbook.cpp",
    "src/Media/Audio/AudioPlayer.cpp",
)

# Printed Prima table-of-contents starts for the nine school chapters.
PRIMA_SCHOOL_PAGES = ((92, 97), (98, 103), (104, 109), (110, 115),
                      (116, 121), (122, 127), (128, 133), (134, 139), (140, 148))


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def spell_text_rows(path: Path) -> dict[int, dict]:
    result = {}
    lines = path.read_bytes().decode("cp1252", errors="replace").splitlines()
    for line_no, row in enumerate(csv.reader(lines, delimiter="\t"), 1):
        if not row or not row[0].strip().isdigit():
            continue
        if len(row) < 12:
            raise ValueError("MM6 spell row is shorter than 12 fields")
        spell_id = int(row[0].strip())
        if spell_id in result:
            raise ValueError("duplicate MM6 spell ID")
        result[spell_id] = {
            "line": line_no,
            "column_count": len(row),
            "name": row[2].strip().strip('"'),
            "damage_tag": row[3].strip(),
            "mana_normal_expert_master": [int(row[i].strip()) for i in (5, 6, 7)],
            "description": row[8].strip().strip('"'),
            "normal_text": row[9].strip().strip('"'),
            "expert_text": row[10].strip().strip('"'),
            "master_text": row[11].strip().strip('"'),
        }
    if sorted(result) != list(range(1, 100)):
        raise ValueError("MM6 spell IDs are not exactly 1 through 99")
    return result


def inherited_spell_data(source: str) -> dict[str, dict]:
    result = {}
    pattern = re.compile(r"\{\s*(SPELL_[A-Z0-9_]+)\s*,\s*SpellData\(([^\n]+)\)\s*\}")
    for match in pattern.finditer(source):
        name = match.group(1)
        fields = [part.strip() for part in match.group(2).split(",")]
        if len(fields) != 12 or not all(re.fullmatch(r"-?\d+", field) for field in fields[:11]):
            raise ValueError(f"unexpected SpellData initializer for {name}")
        result[name] = {
            "line": source.count("\n", 0, match.start()) + 1,
            "mana": list(map(int, fields[:4])),
            "recovery": list(map(int, fields[4:8])),
            "base_damage": int(fields[8]),
            "per_skill_damage": int(fields[9]),
            "flags_initializer": int(fields[10]),
            "required_mastery": fields[11],
        }
    if len(result) != 99:
        raise ValueError(f"expected 99 inherited SpellData rows, found {len(result)}")
    return result


def case_lines(source: str) -> dict[str, list[int]]:
    cases = defaultdict(list)
    for line_no, line in enumerate(source.splitlines(), 1):
        for name in re.findall(r"\bcase\s+(SPELL_[A-Z0-9_]+)\s*:", line):
            cases[name].append(line_no)
    return cases


def enum_number(source: str, name: str) -> int:
    match = re.search(r"\b" + re.escape(name) + r"\s*=\s*(\d+)\b", source)
    if not match:
        raise ValueError(f"missing inherited item enum number {name}")
    return int(match.group(1))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="detailed JSON report inside that directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    reference = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if reference == allowed or not reference.is_relative_to(allowed):
        parser.error("reference must be inside ignored local-game-data/mm6")
    if not report.is_relative_to(reference) or report.suffix.lower() != ".json" or not report.parent.is_dir():
        parser.error("report must be a JSON file in the ignored reference directory")

    private = {
        "spells": reference / "icons/spells.txt",
        "items": reference / "icons/items.txt",
        "item_index": reference / "item-range-probe.json",
        "event_index": reference / "event-probe.json",
        "monster_index": reference / "monster-rules-probe.json",
        "monsters": reference / "icons/monsters.txt",
        "manual": reference / "mm6-manual.pdf",
        "guide": reference / "mm6-prima-guide.pdf",
    }
    sources = {name: repo / name for name in SOURCE_NAMES}
    missing = [str(path) for path in [*private.values(), *sources.values()] if not path.is_file()]
    if missing:
        parser.error("missing local input or tracked source: " + ", ".join(missing))

    spells = spell_text_rows(private["spells"])
    existing = mm6_spell_rows(private["spells"])
    if any(spells[row["id"]]["name"] != row["name"] for row in existing):
        raise ValueError("MM6 spell table parsers disagree")
    items_index = json.loads(private["item_index"].read_text(encoding="utf-8"))
    if items_index.get("input_sha256", {}).get("items") != sha256(private["items"]):
        raise ValueError("retained item-range report is stale")
    items = {row["id"]: row for row in items_index["items"]}
    event_index = json.loads(private["event_index"].read_text(encoding="utf-8"))
    monster_index = json.loads(private["monster_index"].read_text(encoding="utf-8"))
    if monster_index.get("input_sha256", {}).get("icons/monsters.txt") != sha256(private["monsters"]):
        raise ValueError("retained monster-rule report is stale")
    enum = enum_spell_ids(sources["src/Engine/Spells/SpellEnums.h"])
    if sorted(enum) != list(range(1, 100)):
        raise ValueError("inherited spell IDs are not exactly 1 through 99")
    spell_source = sources["src/Engine/Spells/Spells.cpp"].read_text(encoding="utf-8")
    data = inherited_spell_data(spell_source)
    if set(data) != set(enum.values()):
        raise ValueError("inherited spell data and enum differ")
    mm6_loader = spell_source.split("if (gameTitle == GameTitle::MM6) {", 1)[1].split("\n        SpellId uSpellID", 1)[0]
    if "pInfos[spellId]" not in mm6_loader or "pSpellDatas" in mm6_loader:
        raise ValueError("MM6 loader's table mutation contract changed")
    cast_source = sources["src/Engine/Spells/CastSpellInfo.cpp"].read_text(encoding="utf-8")
    cases = case_lines(cast_source)
    target_source = sources["src/Engine/Spells/SpellEnumFunctions.h"].read_text(encoding="utf-8")
    item_target_section = target_source.split("isSpellTargetsItem(", 1)[1].split("}", 1)[0]
    inherited_item_target_names = set(re.findall(r"SPELL_[A-Z0-9_]+", item_target_section))
    if len(inherited_item_target_names) != 4 or not inherited_item_target_names <= set(enum.values()):
        raise ValueError("inherited item-target spell set changed")
    item_enum_source = sources["src/Engine/Objects/ItemEnums.h"].read_text(encoding="utf-8")
    inherited_scroll = [enum_number(item_enum_source, name) for name in
                        ("ITEM_SCROLL_TORCH_LIGHT", "ITEM_SCROLL_SOULDRINKER")]
    inherited_book = [enum_number(item_enum_source, name) for name in
                      ("ITEM_SPELLBOOK_TORCH_LIGHT", "ITEM_SPELLBOOK_SOULDRINKER")]

    label_ids = defaultdict(list)
    for spell_id, name in enum.items():
        label_ids[normalized_name(name.split("_", 2)[2])].append(spell_id)
    retained_name_ids = {normalized_name(row["name"]): spell_id for spell_id, row in spells.items()}
    if len(retained_name_ids) != 99:
        raise ValueError("retained spell names are not unique after normalization")
    monster_source = sources["src/Engine/Objects/Monsters.cpp"].read_text(encoding="utf-8")
    monster_map_source = monster_source.split("monsterSpellMap = {", 1)[1].split("};", 1)[0]
    monster_spell_map = {normalized_name(label): enum_name for label, enum_name in
                         re.findall(r'\{\s*"([^"]+)"\s*,\s*(SPELL_[A-Z0-9_]+)\s*\}', monster_map_source)}
    monster_joins = []
    for monster in monster_index["records"]:
        spell = monster["monster_spell"]
        if not spell:
            continue
        key = normalized_name(spell["label"])
        current_enum = monster_spell_map.get(key)
        current_id = next((spell_id for spell_id, name in enum.items() if name == current_enum), None)
        retained_id = retained_name_ids.get(key)
        kind = ("not_recognized" if current_id is None else
                "recognized_not_retained_label" if retained_id is None else
                "recognized_same_id" if current_id == retained_id else "recognized_different_id")
        monster_joins.append({"monster_id": monster["id"], "spell_label": spell["label"],
                              "current_spell_id": current_id, "retained_label_spell_id": retained_id,
                              "classification": kind})
    active_event_casts = []
    for file in event_index["files"]:
        if file["role"] not in ("listed_map", "global"):
            continue
        path = reference / "icons" / file["name"]
        raw = path.read_bytes()
        if hashlib.sha256(raw).hexdigest() != file["sha256"]:
            raise ValueError("retained event index is stale")
        for event in file["records"]:
            if event["opcode"] != 21:
                continue
            offset, size = event["offset"], event["size"]
            if size != 32 or offset < 0 or offset + size > len(raw) or raw[offset] + 1 != size:
                raise ValueError("active CastSpell event has an unexpected frame")
            spell_id = raw[offset + 5]
            if spell_id not in spells:
                raise ValueError("active CastSpell event has an out-of-range spell ID")
            active_event_casts.append({"file": file["name"], "event_id": event["event_id"],
                                       "offset": offset, "spell_id": spell_id,
                                       "raw_mastery": raw[offset + 6], "raw_skill": raw[offset + 7]})
    records = []
    for spell_id in range(1, 100):
        row = spells[spell_id]
        enum_name = enum[spell_id]
        current = data[enum_name]
        key = normalized_name(row["name"])
        inherited_key = normalized_name(enum_name.split("_", 2)[2])
        label_status = ("same_label" if key == inherited_key else
                        "label_at_other_id" if key in label_ids else "absent_or_renamed")
        scroll_id = 199 + spell_id
        book_id = 299 + spell_id
        if items[scroll_id]["mm6_category"] != "spell_scroll" or items[book_id]["mm6_category"] != "spellbook":
            raise ValueError("inferred MM6 99-item bands are not all spell scrolls/books")
        records.append({
            "spell_id": spell_id,
            "school_block_index": (spell_id - 1) // 11,
            "prima_printed_school_chapter_pages": PRIMA_SCHOOL_PAGES[(spell_id - 1) // 11],
            "retained": row,
            "current_enum_name": enum_name,
            "current_spell_data": current,
            "label_status": label_status,
            "label_at_other_current_ids": label_ids.get(key, []),
            "same_first_three_mana": row["mana_normal_expert_master"] == current["mana"][:3],
            "current_cast_case_lines": cases.get(enum_name, []),
            "current_direct_special_branch": enum_name in ("SPELL_WATER_TOWN_PORTAL", "SPELL_WATER_LLOYDS_BEACON"),
            "current_item_target_branch": enum_name in inherited_item_target_names,
            "current_damage_tag_falls_to_physical": row["damage_tag"].casefold() in {"elec", "cold", "pois"},
            "candidate_scroll_item_id": scroll_id,
            "candidate_spellbook_item_id": book_id,
            "scroll_item_name": items[scroll_id]["name"],
            "spellbook_item_name": items[book_id]["name"],
            "candidate_scroll_in_inherited_range": inherited_scroll[0] <= scroll_id <= inherited_scroll[1],
            "candidate_spellbook_in_inherited_range": inherited_book[0] <= book_id <= inherited_book[1],
            "active_event_casts": [e for e in active_event_casts if e["spell_id"] == spell_id],
            "monster_spell_label_uses": [m for m in monster_joins if m["retained_label_spell_id"] == spell_id],
        })

    summary = {
        "retained_spell_rows": len(records),
        "schools_in_11_spell_blocks": len({r["school_block_index"] for r in records}),
        "label_status": dict(sorted(Counter(r["label_status"] for r in records).items())),
        "same_first_three_mana": sum(r["same_first_three_mana"] for r in records),
        "different_first_three_mana": sum(not r["same_first_three_mana"] for r in records),
        "retained_nonphysical_tags_falling_to_physical": sum(r["current_damage_tag_falls_to_physical"] for r in records),
        "current_grandmaster_required_spells": sum(r["current_spell_data"]["required_mastery"] == "MASTERY_GRANDMASTER" for r in records),
        "current_cast_switch_cases": sum(bool(r["current_cast_case_lines"]) for r in records),
        "current_direct_special_branches_without_case": sum(r["current_direct_special_branch"] and not r["current_cast_case_lines"] for r in records),
        "current_item_target_ids": sum(r["current_item_target_branch"] for r in records),
        "current_item_target_ids_with_different_retained_label": sum(r["current_item_target_branch"] and r["label_status"] != "same_label" for r in records),
        "mm6_scroll_ids_outside_inherited_scroll_range": sum(not r["candidate_scroll_in_inherited_range"] for r in records),
        "mm6_spellbook_ids_outside_inherited_book_range": sum(not r["candidate_spellbook_in_inherited_range"] for r in records),
        "mm6_loader_overrides_static_spell_data": False,
        "active_event_cast_records": len(active_event_casts),
        "active_event_cast_distinct_spell_ids": len({r["spell_id"] for r in active_event_casts}),
        "active_event_casts_with_different_or_absent_inherited_label": sum(
            next(r for r in records if r["spell_id"] == cast["spell_id"])["label_status"] != "same_label"
            for cast in active_event_casts),
        "monster_spell_cells": len(monster_joins),
        "monster_spell_label_joins": dict(sorted(Counter(r["classification"] for r in monster_joins).items())),
    }
    result = {
        "format": 1,
        "scope": "retained MM6 text/index and tracked C++ source; guide/manual files fingerprinted, not transcribed",
        "warning": "A cast case, name match, or item-band join is not spell-effect correctness or live behavior.",
        "summary": summary,
        "inherited_scroll_range": inherited_scroll,
        "inherited_spellbook_range": inherited_book,
        "spells": records,
        "active_event_casts": active_event_casts,
        "monster_spell_joins": monster_joins,
        "input_sha256": {name: sha256(path) for name, path in private.items()},
        "source_sha256": {name: sha256(path) for name, path in sources.items()},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
