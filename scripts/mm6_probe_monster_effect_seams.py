#!/usr/bin/env python3
"""Join retained MM6 monster effect rows to current source hazards.

This F2 probe reads ignored derived reports, never the original installation.
Its per-ID output is restricted to an ignored MM6 research directory.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re


FIXED_DROP_FAMILIES = {
    "MONSTER_TYPE_HARPY": "ITEM_REAGENT_HARPY_FEATHER",
    "MONSTER_TYPE_OOZE": "ITEM_REAGENT_VIAL_OF_OOZE_ENDOPLASM",
    "MONSTER_TYPE_TROLL": "ITEM_REAGENT_VIAL_OF_TROLL_BLOOD",
    "MONSTER_TYPE_DEVIL": "ITEM_REAGENT_VIAL_OF_DEVIL_ICHOR",
    "MONSTER_TYPE_DRAGON": "ITEM_REAGENT_DRAGONS_EYE",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load(path: Path) -> dict:
    return json.loads(path.read_text())


def require(yes: bool, reason: str) -> None:
    if not yes:
        raise ValueError(reason)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    ignored_root = (repo / "local-game-data/mm6").resolve(strict=True)
    reference = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if reference == ignored_root or not reference.is_relative_to(ignored_root):
        parser.error("reference must be inside ignored local-game-data/mm6")
    if not report.is_relative_to(reference) or report.suffix != ".json" or not report.parent.is_dir():
        parser.error("report must be JSON inside the ignored reference directory")

    paths = {
        "table": reference / "icons/monsters.txt",
        "combat": reference / "combat-semantics-probe.json",
        "rules": reference / "monster-rules-probe.json",
        "guide": reference / "monster-guide-semantics-private.json",
        "monsters_cpp": repo / "src/Engine/Objects/Monsters.cpp",
        "actor_cpp": repo / "src/Engine/Objects/Actor.cpp",
        "character_cpp": repo / "src/Engine/Objects/Character.cpp",
        "enum_cpp": repo / "src/Engine/Objects/MonsterEnumFunctions.cpp",
        "enum_h": repo / "src/Engine/Objects/MonsterEnumFunctions.h",
        "item_enums": repo / "src/Engine/Objects/ItemEnums.h",
    }
    combat, rules, guide = (load(paths[key]) for key in ("combat", "rules", "guide"))
    table_hash = sha256(paths["table"])
    require(combat["sha256"]["icons/monsters.txt"] == table_hash, "combat report/table hash mismatch")
    require(rules["input_sha256"]["icons/monsters.txt"] == table_hash, "parser report/table hash mismatch")
    require(guide["summary"]["retained_monsters_sha256"] == table_hash, "guide report/table hash mismatch")
    records = combat["records"]
    require([row["id"] for row in records] == list(range(1, 174)), "expected 173 ordered combat rows")
    guide_rows = {row["id"]: row for row in guide["records"]}
    rules_rows = {row["id"]: row for row in rules["records"]}
    require(set(guide_rows) == set(rules_rows) == set(range(1, 174)), "per-ID report coverage mismatch")

    monsters_cpp = paths["monsters_cpp"].read_text()
    actor_cpp = paths["actor_cpp"].read_text()
    character_cpp = paths["character_cpp"].read_text()
    enum_cpp = paths["enum_cpp"].read_text()
    enum_h = paths["enum_h"].read_text()
    item_enums = paths["item_enums"].read_text()
    require("{'m', ATTACK_PREFERENCE_MONK}" in monsters_cpp, "MM6 M preference parser changed")
    require("info.numCharactersAttackedPerSpecialAbility = lc - '0'" in monsters_cpp,
            "numeric preference parser changed")
    require("baseClass == CLASS_MONK" in character_cpp, "Monk preference consumer changed")
    require("itemDropForMonsterType(monsterTypeForMonsterId(actor->monsterInfo.id))" in actor_cpp,
            "fixed drop consumer changed")
    require("grng->random(100) < 20 && drop.itemId != ITEM_NULL" in actor_cpp,
            "fixed drop chance path changed")
    require("(std::to_underlying(monsterId) - 1) / 3 + 1" in enum_h,
            "inherited monster family formula changed")
    break_weapon = character_cpp.split("case SPECIAL_ATTACK_BREAK_WEAPON:", 1)[1].split(
        "case SPECIAL_ATTACK_STEAL:", 1)[0]
    require("if (!itemstobreaklist.empty()) return 0;" in break_weapon,
            "break-weapon guard changed")
    require("pActor->monsterInfo.aiType == MONSTER_AI_WIMP" in actor_cpp,
            "AI Wimp path changed")

    item_ids = {}
    for family, item in FIXED_DROP_FAMILIES.items():
        require(f"case {family}: return {item};" in enum_cpp, f"fixed-drop family changed: {family}")
        match = re.search(rf"\b{item}\s*=\s*(\d+)", item_enums)
        require(match is not None, f"fixed-drop item ID missing: {item}")
        item_ids[item] = int(match.group(1))

    rows = []
    for row in records:
        number = row["id"]
        preference = row["attack_preference"]
        fixed_item = FIXED_DROP_FAMILIES.get(row["inherited_mm7_family"])
        special = row["special_attack"].casefold()
        rows.append({
            "id": number,
            "guide_page": guide_rows[number]["guide_expected_pdf_page_by_order"],
            "guide_identity_status": guide_rows[number]["guide_identity_status"],
            "attack_preference": preference,
            "has_numeric_multi_target_preference": any(char in "234" for char in preference),
            "has_m_preference": "m" in preference.casefold(),
            "ai": row["flight_movement_ai_hostility_speed_recovery"][2],
            "special_attack": row["special_attack"],
            "has_break_weapon_special": special.startswith("brkweapon"),
            "fixed_drop_inherited_family": row["inherited_mm7_family"] if fixed_item else None,
            "fixed_drop_item_id": item_ids[fixed_item] if fixed_item else None,
            "active_parser_flags": {
                "attack_types": rules_rows[number]["attack_types"],
                "missiles": rules_rows[number]["missiles"],
                "monster_spell": rules_rows[number]["monster_spell"],
                "special_attack": rules_rows[number]["special_attack"],
            },
        })

    count_field = "numCharactersAttackedPerSpecialAbility"
    engine_cpp = list((repo / "src/Engine").rglob("*.cpp"))
    count_mentions = {str(path.relative_to(repo)): path.read_text().count(count_field)
                      for path in engine_cpp if count_field in path.read_text()}
    runtime_readers = [path for path in count_mentions if path not in (
        "src/Engine/Objects/Monsters.cpp", "src/Engine/Snapshots/EntitySnapshots.cpp")]
    summary = {
        "monsters": len(rows),
        "guide_identity_direct_or_prior": sum(r["guide_identity_status"].startswith("direct_page_image") for r in rows),
        "numeric_multi_target_preference_rows": sum(r["has_numeric_multi_target_preference"] for r in rows),
        "m_preference_rows": sum(r["has_m_preference"] for r in rows),
        "break_weapon_special_rows": sum(r["has_break_weapon_special"] for r in rows),
        "fixed_drop_inherited_family_rows": sum(r["fixed_drop_item_id"] is not None for r in rows),
        "fixed_drop_ids_in_mm6_scroll_band": sum(r["fixed_drop_item_id"] is not None
                                              and 200 <= r["fixed_drop_item_id"] <= 298 for r in rows),
        "numeric_preference_runtime_readers_found": len(runtime_readers),
        "existing_parser_summary": rules["summary"],
    }
    require(not runtime_readers, "numeric preference acquired a runtime consumer; review packet")
    report.write_text(json.dumps({
        "format": 1,
        "scope": "retained MM6 reports and current tracked source; no original install or MM6 launch",
        "summary": summary,
        "records": rows,
        "fixed_drop_item_ids": item_ids,
        "numeric_preference_source_mentions": count_mentions,
        "guide_pdf_sha256": guide["summary"]["guide_pdf_sha256"],
        "sha256": {name: sha256(path) for name, path in paths.items()},
    }, indent=2) + "\n")
    print(json.dumps({k: v for k, v in summary.items() if k != "existing_parser_summary"}, sort_keys=True))


if __name__ == "__main__":
    main()
