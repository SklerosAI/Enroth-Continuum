#!/usr/bin/env python3
"""Compare retained MM6 monster labels with the current C++ parser branches.

Read only the already extracted, Git-ignored MM6 table. Keep per-row labels in
the ignored JSON report; print aggregate counts only. A lexical parser match
does not establish correct combat, AI, loot, resistance, or campaign behavior.
"""

import argparse
import csv
import hashlib
import json
import re
from collections import Counter
from pathlib import Path


SOURCE_FILES = (
    "scripts/mm6_probe_monster_rules.py",
    "src/Engine/Objects/Monsters.cpp",
    "src/Engine/Objects/MonsterEnumFunctions.cpp",
    "src/Engine/Objects/MonsterEnumFunctions.h",
    "src/Engine/Tables/HostilityTable.cpp",
    "src/Engine/Objects/Actor.cpp",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cpp_block(source: str, beginning: str, ending: str) -> str:
    if beginning not in source:
        raise ValueError(f"source marker missing: {beginning}")
    block = source.split(beginning, 1)[1]
    if ending not in block:
        raise ValueError(f"source end marker missing: {ending}")
    return block.split(ending, 1)[0]


def current_cpp_labels(source: str) -> dict:
    spell = cpp_block(source, "SpellId ParseSpellType(std::string_view name) {", "CombinedSkillValue ParseSkillValue")
    attack = cpp_block(source, "static DamageType ParseAttackType(std::string_view damage_type_str) {", "void ParseDamage(")
    missile = cpp_block(source, "MonsterProjectile ParseMissleAttackType(std::string_view missle_attack_str) {", "MonsterSpecialAttack ParseSpecialAttack(")
    special = cpp_block(source, "MonsterSpecialAttack ParseSpecialAttack(std::string_view spec_att_str) {", "MonsterId MonsterStats::FindMonsterByInternalName")
    treasure = cpp_block(source, "static const std::map<std::string, RandomItemType", "// \"Imm\" in any of the resistance columns")

    if "return SPELL_NONE;" not in spell or "DAMAGE_PHYSICAL" not in attack or "return MONSTER_PROJECTILE_NONE;" not in missile or "return SPECIAL_ATTACK_NONE;" not in special:
        raise ValueError("monsters.cpp fallback assumptions changed; review the probe")
    if "return valueOr(damageTypeMap, damage_type_str, DAMAGE_PHYSICAL);" not in attack:
        raise ValueError("attack fallback assumption changed; review the probe")

    spell_names = {name.casefold() for name in re.findall(r'\{"([^"]+)",\s*SPELL_', spell)}
    attack_map = {name.casefold(): value for name, value in re.findall(r'\{"([^"]+)",\s*(DAMAGE_[A-Z_]+)\}', attack)}
    missile_names = {name.casefold() for name in re.findall(r'noCaseEquals\(missle_attack_str,\s*"([^"]+)"\)', missile)}
    special_prefixes = tuple(name.casefold() for name in re.findall(r'tmp\.starts_with\("([^"]+)"\)', special))
    treasure_names = {name.casefold() for name in re.findall(r'\{"([^"]+)",\s*RANDOM_', treasure)}
    if not all((spell_names, attack_map, missile_names, special_prefixes, treasure_names)):
        raise ValueError("could not extract a C++ parser vocabulary")
    return {"spell": spell_names, "attack": attack_map, "missile": missile_names,
            "special": special_prefixes, "treasure": treasure_names}


def cell(row: list[str], index: int) -> str:
    return row[index].strip().strip('"')


def special_base(label: str) -> str:
    # Mirrors Monsters.cpp's parseSpecialAttack: take the substring before the
    # first case-insensitive 'x', which may expose a shipped spelling mismatch.
    lower = label.casefold()
    return lower.split("x", 1)[0]


def treasure_category(label: str) -> str:
    # Mirrors parseTreasure's first 'L' separator and single level digit.
    positions = [p for p in (label.find("l"), label.find("L")) if p >= 0]
    if not positions:
        return ""
    part = label[min(positions) + 1:]
    return part[1:] if part and part[0].isdigit() else ""


def inspect_row(row: list[str], labels: dict) -> dict:
    result = {"id": int(cell(row, 0)), "attack_types": [], "missiles": [],
              "monster_spell": None, "special_attack": None, "treasure_category": None}
    for column, slot in ((17, "primary"), (21, "secondary")):
        name = cell(row, column)
        if name in ("", "0"):
            continue
        mapping = labels["attack"].get(name.casefold())
        result["attack_types"].append({"slot": slot, "label": name,
                                       "mapping": mapping or "DAMAGE_PHYSICAL_FALLBACK",
                                       "physical_fallback": mapping is None,
                                       "energy_as_earth": name.casefold() == "ener" and mapping == "DAMAGE_EARTH"})
    for column, slot in ((19, "primary"), (23, "secondary")):
        name = cell(row, column)
        if name in ("", "0"):
            continue
        result["missiles"].append({"slot": slot, "label": name,
                                   "recognized": name.casefold() in labels["missile"]})
    spell_cell = cell(row, 25)
    if spell_cell not in ("", "0"):
        name = spell_cell.split(",", 1)[0].strip()
        result["monster_spell"] = {"label": name, "recognized": name.casefold() in labels["spell"]}
    special = cell(row, 16)
    if len(special) > 1:
        base = special_base(special)
        result["special_attack"] = {"label": special,
                                    "recognized": any(base.startswith(prefix) for prefix in labels["special"])}
    category = treasure_category(cell(row, 7))
    if category:
        result["treasure_category"] = {"label": category,
                                       "recognized": category.casefold() in labels["treasure"]}
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="retained ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="detailed JSON report inside that directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if not root.is_relative_to(allowed) or root == allowed:
        parser.error("reference must be an ignored MM6 research directory")
    if not report.is_relative_to(root) or report.suffix.lower() != ".json" or not report.parent.is_dir():
        parser.error("JSON report must stay inside the ignored MM6 research directory")

    monsters = root / "icons/monsters.txt"
    if not monsters.is_file():
        parser.error("retained monsters.txt is required")
    source = repo / "src/Engine/Objects/Monsters.cpp"
    labels = current_cpp_labels(source.read_text(encoding="utf-8"))
    records = []
    failures = []
    ids = set()
    for line, row in enumerate(csv.reader(monsters.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t"), 1):
        if not row or not row[0].strip().isdigit():
            continue
        monster_id = int(row[0].strip())
        if len(row) != 33:
            failures.append({"line": line, "id": monster_id, "reason": "wrong_column_count", "columns": len(row)})
            continue
        if monster_id in ids:
            failures.append({"line": line, "id": monster_id, "reason": "duplicate_id"})
            continue
        ids.add(monster_id)
        record = inspect_row(row, labels)
        record["line"] = line
        records.append(record)
    if ids and ids != set(range(min(ids), max(ids) + 1)):
        failures.append({"reason": "noncontiguous_ids"})

    attacks = [attack for record in records for attack in record["attack_types"]]
    missiles = [missile for record in records for missile in record["missiles"]]
    spells = [record["monster_spell"] for record in records if record["monster_spell"]]
    specials = [record["special_attack"] for record in records if record["special_attack"]]
    treasure = [record["treasure_category"] for record in records if record["treasure_category"]]
    summary = {
        "monster_rows": len(records), "distinct_ids": len(ids),
        "nonzero_attack_type_cells": len(attacks),
        "attack_type_physical_fallbacks": sum(a["physical_fallback"] for a in attacks),
        "energy_labels_mapped_to_earth": sum(a["energy_as_earth"] for a in attacks),
        "monster_rows_with_attack_type_mismatch": sum(any(a["physical_fallback"] or a["energy_as_earth"] for a in record["attack_types"]) for record in records),
        "nonzero_missile_cells": len(missiles),
        "missile_labels_mapped_to_none": sum(not m["recognized"] for m in missiles),
        "nonzero_monster_spell_cells": len(spells),
        "monster_spell_labels_mapped_to_none": sum(not s["recognized"] for s in spells),
        "distinct_unmapped_monster_spell_labels": len({s["label"].casefold() for s in spells if not s["recognized"]}),
        "nonzero_special_attack_cells": len(specials),
        "special_attack_labels_mapped_to_none": sum(not s["recognized"] for s in specials),
        "treasure_category_cells": len(treasure),
        "unmapped_treasure_categories": sum(not t["recognized"] for t in treasure),
        "structural_failures": len(failures),
    }
    result = {
        "format": 1,
        "scope": "retained MM6 monsters.txt and tracked C++ parser only; no original archive read",
        "interpretation": "Lexical classification is not proof of correct gameplay, combat formulas, loot, or save behavior.",
        "summary": summary,
        "records": records,
        "structural_failures": failures,
        "input_sha256": {"icons/monsters.txt": sha256(monsters)},
        "source_sha256": {name: sha256(repo / name) for name in SOURCE_FILES},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
