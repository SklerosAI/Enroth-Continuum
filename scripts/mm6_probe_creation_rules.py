#!/usr/bin/env python3
"""Compare inherited party-creation literals with retained MM6 item/spell bands.

This is an F2 source/data join. Exact item and spell names and row identities
remain in an ignored local JSON report. It does not prove a playable new game.
"""

import argparse
import hashlib
import json
import re
from pathlib import Path


SOURCES = (
    "scripts/mm6_probe_creation_rules.py",
    "src/Engine/Party.cpp",
    "src/Engine/Objects/Character.cpp",
    "src/Engine/Objects/CharacterEnums.h",
    "src/Engine/Objects/ItemEnums.h",
    "src/GUI/UI/UIPartyCreation.cpp",
    "src/Application/Game.cpp",
    "src/Application/Startup/GameStarter.cpp",
    "src/Application/Startup/GameStarterOptions.h",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cpp_enum_values(source: str, prefix: str) -> dict[str, int]:
    return {name: int(value) for name, value in re.findall(
        r"^\s*(" + re.escape(prefix) + r"[A-Z0-9_]+)\s*=\s*(\d+)\s*,",
        source, re.MULTILINE)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored retained MM6 research directory")
    parser.add_argument("report", type=Path, help="JSON report inside that directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    reference = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if reference == allowed or not reference.is_relative_to(allowed):
        parser.error("reference must be an ignored MM6 research directory")
    if not report.is_relative_to(reference) or report.suffix.lower() != ".json" or not report.parent.is_dir():
        parser.error("report must stay inside the ignored research directory")

    source_paths = {name: repo / name for name in SOURCES}
    inputs = {name: reference / name for name in (
        "item-range-probe.json", "spell-effect-probe.json",
        "mm6-manual.pdf", "mm6-reference-card.pdf", "mm6-prima-guide.pdf",
        "icons/items.txt", "icons/spells.txt")}
    missing = [str(path) for path in [*source_paths.values(), *inputs.values()] if not path.is_file()]
    if missing:
        parser.error("missing source or retained reference: " + ", ".join(missing))

    items_report = json.loads(inputs["item-range-probe.json"].read_text(encoding="utf-8"))
    spells_report = json.loads(inputs["spell-effect-probe.json"].read_text(encoding="utf-8"))
    if items_report.get("input_sha256", {}).get("items") != sha256(inputs["icons/items.txt"]):
        raise ValueError("item-range report is stale against retained items.txt")
    spell_inputs = spells_report.get("input_sha256", {})
    if (spell_inputs.get("spells") != sha256(inputs["icons/spells.txt"]) or
            spell_inputs.get("item_index") != sha256(inputs["item-range-probe.json"])):
        raise ValueError("spell-effect report is stale against retained inputs")
    if spells_report.get("summary", {}).get("retained_spell_rows") != 99:
        raise ValueError("spell-effect report does not cover 99 MM6 IDs")
    item_rows = {row["id"]: row for row in items_report["items"]}
    item_enum = cpp_enum_values(source_paths["src/Engine/Objects/ItemEnums.h"].read_text(encoding="utf-8"), "ITEM_")
    spell_rows = {row["current_enum_name"]: row for row in spells_report["spells"]}

    ui = source_paths["src/GUI/UI/UIPartyCreation.cpp"].read_text(encoding="utf-8")
    if "for (Skill j : allSkills())" not in ui or "for (InventoryEntry entry" not in ui:
        raise ValueError("party creation startup item section moved")
    startup = ui.split("for (Skill j : allSkills())", 1)[1].split("for (InventoryEntry entry", 1)[0]
    startup = "\n".join(line.split("//", 1)[0] for line in startup.splitlines())
    literal_names = sorted(set(re.findall(r"\bItem\((ITEM_[A-Z0-9_]+)\)", startup)))
    if not literal_names or any(name not in item_enum for name in literal_names):
        raise ValueError("startup item literals cannot be resolved")
    literal_items = []
    for name in literal_names:
        item_id = item_enum[name]
        if item_id not in item_rows:
            raise ValueError("startup item constant outside retained item table")
        literal_items.append({"source_constant": name, "source_numeric_id": item_id,
                              "retained_name": item_rows[item_id]["name"],
                              "retained_category": item_rows[item_id]["mm6_category"],
                              "retained_type": item_rows[item_id]["mm6_type"]})
    inherited_books = [row for row in literal_items if row["source_constant"].startswith("ITEM_SPELLBOOK_")]
    inherited_bottle = next((row for row in literal_items if row["source_constant"] == "ITEM_POTION_BOTTLE"), None)
    if inherited_bottle is None:
        raise ValueError("inherited bottle literal missing")

    unlock_names = sorted(set(re.findall(r"bHaveSpell\[(SPELL_[A-Z0-9_]+)\]", startup)))
    if any(name not in spell_rows for name in unlock_names):
        raise ValueError("startup spell unlock cannot be resolved")
    spell_unlocks = [{"source_constant": name, "numeric_id": spell_rows[name]["spell_id"],
                      "retained_name": spell_rows[name]["retained"]["name"],
                      "label_status": spell_rows[name]["label_status"]} for name in unlock_names]

    ui_class_section = ui.split("CreateButton({323, 417}", 1)[1].split("pPlayerCreationUI_BtnOK", 1)[0]
    class_buttons = re.findall(r"UIMSG_PlayerCreationSelectClass,\s*(?:0x[0-9A-Fa-f]+|\d+)\)", ui_class_section)
    if len(class_buttons) != 9:
        raise ValueError("inherited party creation class-button count changed")
    party = source_paths["src/Engine/Party.cpp"].read_text(encoding="utf-8")
    reset = party.split("void Party::Reset()", 1)[1].split("void Party::yell()", 1)[0]
    if "ChangeClass(CLASS_THIEF)" not in reset:
        raise ValueError("inherited reset class seed changed")
    game = source_paths["src/Application/Game.cpp"].read_text(encoding="utf-8")
    if "if (!PartyCreationUI_Loop())" not in game:
        raise ValueError("new-game creation routing changed")

    summary = {
        "startup_literal_item_constants": len(literal_items),
        "startup_spellbook_constants": len(inherited_books),
        "startup_spellbook_constants_outside_mm6_book_band": sum(not 300 <= row["source_numeric_id"] <= 398 for row in inherited_books),
        "startup_spellbook_constants_not_mm6_spellbooks": sum(row["retained_category"] != "spellbook" for row in inherited_books),
        "inherited_bottle_numeric_id_in_mm6_scroll_band": 200 <= inherited_bottle["source_numeric_id"] <= 298,
        "inherited_bottle_retained_category": inherited_bottle["retained_category"],
        "startup_spell_unlocks": len(spell_unlocks),
        "startup_spell_unlocks_with_different_or_absent_retained_label": sum(row["label_status"] != "same_label" for row in spell_unlocks),
        "inherited_class_buttons": len(class_buttons),
        "current_reset_seeds_mm7_thief": True,
        "current_new_game_always_enters_party_creation": True,
    }
    payload = {
        "format": 1,
        "scope": "current source and retained MM6 indexes; no original archive or runtime opened",
        "warning": "Source item/spell literals and lexical categories do not establish original MM6 starting inventory or effects.",
        "summary": summary,
        "startup_literal_items": literal_items,
        "startup_spell_unlocks": spell_unlocks,
        "source_sha256": {name: sha256(path) for name, path in source_paths.items()},
        "input_sha256": {name: sha256(path) for name, path in inputs.items()},
    }
    report.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
