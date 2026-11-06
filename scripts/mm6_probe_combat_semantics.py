#!/usr/bin/env python3
"""Inventory all retained MM6 monster combat rows without publishing row data.

This is an F2 inventory, not an original-archive preflight or runtime test.
Exact per-ID fields stay in an ignored local-game-data/mm6 JSON report.
"""

import argparse
import csv
import hashlib
import json
import re
from collections import Counter
from pathlib import Path


SOURCES = (
    "src/Engine/Objects/Monsters.cpp",
    "src/Engine/Objects/MonsterEnums.h",
    "src/Engine/Objects/MonsterEnumFunctions.h",
    "src/Engine/Objects/MonsterEnumFunctions.cpp",
    "src/Engine/Objects/Actor.cpp",
    "src/Engine/Objects/Character.cpp",
    "src/Engine/Tables/HostilityTable.cpp",
)
RESISTANCE_NAMES = ("fire", "electricity", "cold", "poison", "physical", "magic")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def clean(cell):
    return cell.strip().strip('"').strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="retained ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="JSON output within reference directory")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    reference = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if reference == allowed or not reference.is_relative_to(allowed):
        parser.error("reference must be an ignored MM6 research directory")
    if report.suffix != ".json" or not report.is_relative_to(reference) or not report.parent.is_dir():
        parser.error("report must be JSON inside the ignored reference directory")

    monster_path = reference / "icons/monsters.txt"
    if not monster_path.is_file():
        parser.error("retained icons/monsters.txt is required")
    enum_source = (repo / "src/Engine/Objects/MonsterEnums.h").read_text()
    enum_block = enum_source.split("enum class MonsterType {", 1)[1].split("};", 1)[0]
    mm7_types = re.findall(r"^\s*(MONSTER_TYPE_[A-Z0-9_]+)(?:\s*=\s*\d+)?\s*,", enum_block, re.M)
    mm7_types = [name for name in mm7_types if name != "MONSTER_TYPE_INVALID"]
    if not mm7_types or mm7_types[0] != "MONSTER_TYPE_ANGEL":
        parser.error("MM7 monster type enumeration changed; review this inventory")
    monster_source = (repo / "src/Engine/Objects/Monsters.cpp").read_text()
    hostility_source = (repo / "src/Engine/Tables/HostilityTable.cpp").read_text()
    actor_source = (repo / "src/Engine/Objects/Actor.cpp").read_text()
    for needle, source in (
        ('tokens[32] = mm6[31]', monster_source),
        ('tokens[28] = mm6[26]', monster_source),
        ('line.fill(HOSTILITY_FRIENDLY)', hostility_source),
        ('itemDropForMonsterType(monsterTypeForMonsterId', actor_source),
    ):
        if needle not in source:
            parser.error(f"source assumption changed: {needle}")

    rows = []
    errors = []
    ids = set()
    for line_no, row in enumerate(csv.reader(monster_path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t"), 1):
        if not row or not row[0].strip().isdigit():
            continue
        monster_id = int(row[0].strip())
        if len(row) != 33 or monster_id in ids:
            errors.append({"line": line_no, "id": monster_id, "columns": len(row), "duplicate": monster_id in ids})
            continue
        ids.add(monster_id)
        rows.append((line_no, [clean(value) for value in row]))
    if ids != set(range(1, 174)):
        errors.append({"reason": "expected_contiguous_ids_1_to_173"})

    records = []
    for line_no, row in rows:
        monster_id = int(row[0])
        family_index = (monster_id - 1) // 3
        records.append({
            "id": monster_id, "line": line_no,
            "mm6_picture": row[1], "mm6_display_name": row[2],
            "numeric_family": family_index + 1, "numeric_tier": (monster_id - 1) % 3,
            "inherited_mm7_family": mm7_types[family_index] if family_index < len(mm7_types) else None,
            "level_hp_ac_exp": [row[i] for i in (3, 4, 5, 6)],
            "treasure": row[7], "flight_movement_ai_hostility_speed_recovery": [row[i] for i in (9, 10, 11, 12, 13, 14)],
            "attack_preference": row[15], "special_attack": row[16],
            "primary": [row[i] for i in (17, 18, 19)],
            "secondary": [row[i] for i in (20, 21, 22, 23)],
            "spell": [row[i] for i in (24, 25)],
            "resistances": dict(zip(RESISTANCE_NAMES, (row[i] for i in range(26, 32)))),
            "special_ability": row[32],
        })
    families = Counter(record["numeric_family"] for record in records)
    active = lambda value: value not in ("", "0")
    summary = {
        "rows": len(records), "families_by_numeric_triplet": len(families),
        "complete_triplets": sum(count == 3 for count in families.values()),
        "partial_triplets": sum(count != 3 for count in families.values()),
        "rows_with_secondary_chance": sum(active(record["secondary"][0]) for record in records),
        "rows_with_spell_chance": sum(active(record["spell"][0]) for record in records),
        "rows_with_treasure": sum(active(record["treasure"]) for record in records),
        "rows_with_attack_preference": sum(active(record["attack_preference"]) for record in records),
        "rows_with_special_attack": sum(active(record["special_attack"]) for record in records),
        "rows_with_special_ability": sum(active(record["special_ability"]) for record in records),
        "rows_flying": sum(record["flight_movement_ai_hostility_speed_recovery"][0] == "Y" for record in records),
        "resistance_nonzero_cells": {name: sum(active(record["resistances"][name]) for record in records) for name in RESISTANCE_NAMES},
        "resistance_immune_cells": {name: sum(record["resistances"][name].casefold() == "imm" for record in records) for name in RESISTANCE_NAMES},
        "errors": len(errors),
    }
    payload = {
        "format": 1,
        "scope": "retained extracted MM6 monsters.txt and tracked source only; no original archive read or game launch",
        "interpretation": "Numeric family is not a verified semantic identity; current inherited MM7 family is shown for audit, not use.",
        "summary": summary, "records": records, "errors": errors,
        "sha256": {"icons/monsters.txt": digest(monster_path), **{name: digest(repo / name) for name in SOURCES}},
    }
    report.write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    if errors:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
