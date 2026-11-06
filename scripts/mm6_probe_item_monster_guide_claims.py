#!/usr/bin/env python3
"""Build a private per-ID MM6 item/monster guide reconciliation queue.

This consumes retained, ignored probe reports only. Guide page ranges are
candidate chapter locators, not assertions that an individual row was checked.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import unicodedata
from collections import Counter
from pathlib import Path


def guide_family(item: dict) -> tuple[str, str]:
    item_id = item["id"]
    kind = item["mm6_type"]
    if kind == "WeaponW":
        return "wands", "201-203"
    if kind == "Herb":
        return "herbs", "203-204"
    if kind == "Bottle":
        return "potions and alchemy", "204-205, 236-240"
    if kind == "Sscroll":
        return "spell scrolls", "206-208"
    if kind == "Book":
        return "spellbooks", "209-212"
    if kind == "Mscroll":
        return "quest and miscellaneous documents", "224-231"
    if item["rarity_field"] == "Artifact":
        return "artifacts", "212-218"
    if item["rarity_field"] == "Relic":
        return "relics", "218-224"
    if 400 <= item_id <= 499:
        return "other special, quest or miscellaneous item (unresolved)", "212-231"
    if item_id >= 500:
        return "quest or miscellaneous item (unresolved)", "224-231"
    if kind in {"Weapon", "Weapon1or2", "Weapon2", "Missile"}:
        return "weapons", "159-180"
    if kind in {"Armor", "Shield", "Helm", "Boots", "Cloak", "cloak", "Belt", "Gauntlets", "Ring", "Amulet"}:
        return "bodily protection and jewelry", "180-200"
    return "unclassified item (requires direct guide/data join)", "157-240"


def normalized(text: str) -> str:
    """Normalize OCR/name punctuation while keeping word boundaries."""
    return " ".join(re.findall(r"[a-z0-9]+", unicodedata.normalize("NFKD", text).casefold()))


def raw_text_pages(path: Path) -> dict[int, str]:
    text = path.read_text()
    pages = {
        int(page): body
        for page, body in re.findall(
            r"--- PDF page (\d+) ---\n(.*?)(?=\n--- PDF page|\Z)", text, re.S
        )
    }
    if sorted(pages) != list(range(1, 413)):
        raise ValueError("retained Prima text must contain exactly 412 marked pages")
    return pages


def retained_item_values(path: Path) -> dict[int, int | None]:
    values = {}
    with path.open(encoding="latin-1", newline="") as source:
        for cells in csv.reader(source, delimiter="\t"):
            if len(cells) >= 4 and cells[0].isdigit():
                values[int(cells[0])] = int(cells[3].replace(",", "")) if cells[3].strip() else None
    if sorted(values) != list(range(581)):
        raise ValueError("retained MM6 items.txt value IDs are incomplete")
    return values


def retained_item_roles(path: Path) -> dict[int, dict]:
    roles = {}
    with path.open(encoding="latin-1", newline="") as source:
        for cells in csv.reader(source, delimiter="\t"):
            if len(cells) >= 16 and cells[0].isdigit():
                roles[int(cells[0])] = {
                    "effect_token": cells[6].strip(),
                    "quest_description_marker": cells[15].lstrip().casefold().startswith("quest item:"),
                    "schema_cells": {
                        "icon": cells[1], "name": cells[2], "base_value": cells[3],
                        "type": cells[4], "skill": cells[5],
                        "damage_or_effect": cells[6], "damage_modifier": cells[7],
                        "rarity": cells[8], "identify_repair": cells[9],
                        "unidentified_name": cells[10], "sprite_id": cells[11],
                        "paperdoll_x": cells[13], "paperdoll_y": cells[14],
                        "description_present": bool(cells[15].strip()),
                    },
                }
    if sorted(roles) != list(range(581)):
        raise ValueError("retained MM6 items.txt role IDs are incomplete")
    return roles


def source_wand_spells(root: Path) -> tuple[dict[int, int], dict[str, str]]:
    item_path = root / "src/Engine/Objects/ItemEnums.h"
    spell_path = root / "src/Engine/Spells/SpellEnums.h"
    mapping_path = root / "src/Engine/Objects/ItemEnumFunctions.cpp"
    item_text, spell_text, mapping_text = (path.read_text() for path in (item_path, spell_path, mapping_path))
    item_enum = {name: int(value) for name, value in re.findall(r"\b(ITEM_[A-Z0-9_]+)\s*=\s*(\d+)", item_text)}
    spell_enum = {name: int(value) for name, value in re.findall(r"\b(SPELL_[A-Z0-9_]+)\s*=\s*(\d+)", spell_text)}
    inherited_boundaries = {
        "ITEM_WAND_OF_FIRE": 135, "ITEM_MYSTIC_WAND_OF_INCINERATION": 159,
        "ITEM_SCROLL_TORCH_LIGHT": 300, "ITEM_SCROLL_SOULDRINKER": 398,
        "ITEM_SPELLBOOK_TORCH_LIGHT": 400, "ITEM_SPELLBOOK_SOULDRINKER": 498,
        "ITEM_ARTIFACT_PUCK": 500, "ITEM_RELIC_MEKORIGS_HAMMER": 528,
        "ITEM_SPECIAL_GROGNARDS_CUTLASS": 552,
        "ITEM_QUEST_HEART_OF_THE_WOOD": 600, "ITEM_699": 699,
        "ITEM_MESSAGE_FROM_ERATHIA": 700, "ITEM_781": 781,
    }
    if any(item_enum.get(name) != value for name, value in inherited_boundaries.items()):
        raise ValueError("inherited ItemId boundaries changed; review source predicates")
    block = mapping_text.split("spellByWand = {", 1)[1].split("};", 1)[0]
    mapping = {
        item_enum[item]: spell_enum[spell]
        for item, spell in re.findall(r"\{(ITEM_[A-Z0-9_]+),\s*(SPELL_[A-Z0-9_]+)\}", block)
    }
    if sorted(mapping) != list(range(135, 160)):
        raise ValueError("inherited wand mapping does not cover retained MM6 wand IDs")
    consumer_paths = (
        item_path, spell_path, mapping_path,
        root / "src/Engine/Objects/ItemEnumFunctions.h",
        root / "src/Engine/Objects/Item.cpp",
        root / "src/Engine/Objects/Character.cpp",
        root / "src/Engine/Tables/MM6ItemTable.cpp",
        root / "src/Engine/Tables/ItemTable.cpp",
        root / "src/Engine/Evt/EvtVariables.cpp",
        root / "src/GUI/UI/UIMessageScroll.cpp",
    )
    fingerprints = {
        path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in consumer_paths
    }
    return mapping, fingerprints


def retained_held_item_refs(path: Path) -> tuple[dict[int, list[dict]], str]:
    report = json.loads(path.read_text())
    refs: dict[int, list[dict]] = {}
    for link in report["links"]:
        if link["kind"] == "event_variable_operand" and link["variable_id"] == 0x11:
            item_id = link["operand"]
            if 0 <= item_id <= 580:
                refs.setdefault(item_id, []).append({
                    "event": link["event"], "opcode": link["opcode"], "offset": link["offset"]
                })
    return refs, hashlib.sha256(path.read_bytes()).hexdigest()


def retained_role(item: dict, quest_marker: bool) -> str:
    kind = item["mm6_type"]
    if kind == "WeaponW":
        return "wand"
    if kind == "Sscroll":
        return "spell_scroll"
    if kind == "Book":
        return "spellbook"
    if item["rarity_field"] == "Artifact":
        return "artifact_equipment"
    if item["rarity_field"] == "Relic":
        return "relic_equipment"
    if kind == "Mscroll":
        return "message_scroll"
    if item["id"] >= 400:
        equipment = kind != "N / A"
        if quest_marker:
            return "quest_described_equipment" if equipment else "quest_described_misc"
        return "special_equipment_role_unresolved" if equipment else "special_misc_role_unresolved"
    return "outside_high_risk_scope"


def source_role_consumer(role: str) -> tuple[str, str]:
    if role == "wand":
        return "Character attack -> spellForWand; Item::postGenerate charges", "source_spell_id_comparison"
    if role == "spell_scroll":
        return "Character item use -> spellForScroll -> CastSpellInfo", "inherited_spell_lookup_range_300_398"
    if role == "spellbook":
        return "Character item use -> spellForSpellbook -> spell mastery; spellbooksForSchool", "inherited_spell_lookup_range_400_498"
    if role in {"artifact_equipment", "relic_equipment"}:
        return "Item::rarity/equipment; isArtifact ID predicate; PopulateArtifactBonusMap; ItemTable generation", "inherited_artifact_range_500_552"
    if role == "message_scroll":
        return "Character item use -> CreateMsgScrollWindow; isMessageScroll ID predicate; Item::canSellRepairIdentifyAt", "inherited_message_scroll_range_700_781"
    if role != "outside_high_risk_scope":
        return "EvtVariables held-item operand; Item type/use/shop; isQuestItem ID predicate", "inherited_quest_range_600_699"
    return "MM6ItemTable load; shared ItemTable/ItemEnumFunctions/UIPopup behavior requires title check", "outside_high_risk_scope"


def retained_schema_consumer(item: dict, role: str) -> str:
    """Current C++ path selected by the retained row, not behavior proof."""
    if not item["name"]:
        return "blank-name retained row; no named item consumer mapped"
    if role != "outside_high_risk_scope":
        return source_role_consumer(role)[0]
    kind = item["mm6_type"].casefold()
    if kind in {"weapon", "weapon1or2", "weapon2", "missile"}:
        return "MM6ItemTable template; ItemTable generation; Character equip/attack"
    if kind in {"armor", "shield", "helm", "boots", "cloak", "belt", "gauntlets", "ring", "amulet"}:
        return "MM6ItemTable template; ItemTable generation; Character equip/AC/bonuses"
    if kind in {"bottle", "herb"}:
        return "MM6ItemTable template; Character item use; UIPopup potion/mixture path"
    if kind == "gold":
        return "MM6ItemTable template; Item gold type; party gold pickup and value path"
    return "MM6ItemTable template; Item type/display; ItemTable generation or event item path requires title check"


def value_claims(name: str, page_spec: str, pages: dict[int, str]) -> list[dict]:
    """Extract only exact-name plus nearby numeric value claims from OCR."""
    if not name:
        return []
    escaped = re.escape(name)
    block = re.compile(r"^\s*" + escaped + r"\s*\n\s*Value:\s*([\d,]+)\b", re.I | re.M)
    table = re.compile(r"^\s*" + escaped + r"\s+([\d,]+)\s+\d\b", re.I)
    found = set()
    for page in candidate_pages(page_spec):
        body = pages[page]
        for match in block.finditer(body):
            found.add((page, int(match.group(1).replace(",", "")), "labeled_value"))
        for line in body.splitlines():
            match = table.match(line)
            if match:
                found.add((page, int(match.group(1).replace(",", "")), "table_value"))
    return [
        {"pdf_page": page, "value": value, "method": method}
        for page, value, method in sorted(found)
    ]


def candidate_pages(spec: str) -> set[int]:
    pages: set[int] = set()
    for first, last in re.findall(r"(\d+)(?:-(\d+))?", spec):
        pages.update(range(int(first), int(last or first) + 1))
    return pages


def page_hits(name: str, pages: dict[int, str], first: int, last: int) -> list[int]:
    normalized_name = normalized(name)
    if not normalized_name:
        return []
    pattern = re.compile(r"(?<![a-z0-9])" + re.escape(normalized_name) + r"(?![a-z0-9])")
    return [page for page in range(first, last + 1) if pattern.search(pages[page])]


def catalog_status(name: str, within: list[int], outside: list[int]) -> str:
    """Status describes exact OCR name occurrence only, never a semantic join."""
    if not normalized(name):
        return "unreviewed"
    if outside or len(within) > 1 or (len(normalized(name).split()) == 1 and within):
        return "ambiguous"
    if len(within) == 1:
        return "matched"
    return "absent"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    private_root = (root / "local-game-data/mm6").resolve()
    reference = args.reference.resolve()
    report_path = args.report.resolve()
    if not reference.is_relative_to(private_root) or not report_path.is_relative_to(private_root):
        parser.error("reference and report must stay in ignored local-game-data/mm6")
    if report_path.parent != reference:
        parser.error("report must be written directly in reference directory")

    item_report = json.loads((reference / "item-range-probe.json").read_text())
    monster_report = json.loads((reference / "combat-semantics-probe.json").read_text())
    guide_report = json.loads((reference / "guide-coverage-probe.json").read_text())
    prima_pdf = reference / "mm6-prima-guide.pdf"
    prima_hash = hashlib.sha256(prima_pdf.read_bytes()).hexdigest()
    if prima_hash != guide_report["sources"]["prima"]["pdf_sha256"]:
        raise ValueError("retained Prima PDF differs from page coverage report")
    prima_text_path = reference / "mm6-prima-guide.txt"
    raw_pages = raw_text_pages(prima_text_path)
    pages = {page: normalized(body) for page, body in raw_pages.items()}
    item_values_path = reference / "icons/items.txt"
    item_values = retained_item_values(item_values_path)
    item_roles = retained_item_roles(item_values_path)
    wand_spells, source_fingerprints = source_wand_spells(root)
    held_item_refs, quest_graph_hash = retained_held_item_refs(reference / "quest-graph-probe.json")
    overrides_path = reference / "item-monster-guide-reviewed-overrides.json"
    overrides = json.loads(overrides_path.read_text()) if overrides_path.is_file() else {}
    if overrides and overrides.get("prima_pdf_sha256") != guide_report["sources"]["prima"]["pdf_sha256"]:
        raise ValueError("reviewed overrides are for a different Prima PDF")
    items = item_report["items"]
    monsters = monster_report["records"]
    assert [item["id"] for item in items] == list(range(581))
    assert [monster["id"] for monster in monsters] == list(range(1, 174))
    assert guide_report["sources"]["prima"]["page_count"] == 412
    if set(overrides.get("items", {})) - {str(item["id"]) for item in items}:
        raise ValueError("visual item override references absent retained ID")
    if set(overrides.get("monsters", {})) - {str(monster["id"]) for monster in monsters}:
        raise ValueError("visual monster override references absent retained ID")

    item_records = []
    for item in items:
        family, page_spec = guide_family(item)
        item_id = item["id"]
        role_data = item_roles[item_id]
        role = retained_role(item, role_data["quest_description_marker"])
        consumer, source_dispatch_basis = source_role_consumer(role)
        spell_token = role_data["effect_token"]
        retained_spell_id = int(spell_token[1:]) if role in {"wand", "spell_scroll", "spellbook"} and re.fullmatch(r"S\d+", spell_token) else None
        if role in {"wand", "spell_scroll", "spellbook"} and retained_spell_id is None:
            raise ValueError(f"missing retained spell token for item {item_id}")
        if role == "spell_scroll" and retained_spell_id != item_id - 199:
            raise ValueError(f"nonsequential MM6 scroll spell ID at {item_id}")
        if role == "spellbook" and retained_spell_id != item_id - 299:
            raise ValueError(f"nonsequential MM6 spellbook spell ID at {item_id}")
        source_wand_spell_id = wand_spells[item_id] if role == "wand" else None
        if role == "wand":
            source_dispatch_status = "wand_spell_id_matches_retained_token" if source_wand_spell_id == retained_spell_id else "wand_spell_id_disagrees_with_retained_token"
        elif role == "spell_scroll":
            source_dispatch_status = "retained_scroll_id_outside_inherited_lookup_range"
        elif role == "spellbook":
            source_dispatch_status = "retained_book_id_outside_inherited_lookup_range"
        elif role in {"artifact_equipment", "relic_equipment"}:
            source_dispatch_status = "retained_rarity_not_in_inherited_artifact_id_range"
        elif role == "message_scroll":
            source_dispatch_status = "retained_message_id_outside_inherited_message_range"
        elif role != "outside_high_risk_scope":
            source_dispatch_status = "retained_special_id_outside_inherited_quest_range"
        else:
            source_dispatch_status = "not_audited_in_high_risk_pass"
        all_hits = page_hits(item["name"], pages, 157, 240)
        expected = candidate_pages(page_spec)
        within = [page for page in all_hits if page in expected]
        outside = [page for page in all_hits if page not in expected]
        status = catalog_status(item["name"], within, outside)
        reviewed = overrides.get("items", {}).get(str(item["id"]))
        if reviewed:
            if reviewed["status"] not in {"matched", "ambiguous", "absent", "generic_category_only", "not_listed_in_printed_catalog"}:
                raise ValueError("invalid visual item override status")
            if reviewed["status"] in {"generic_category_only", "not_listed_in_printed_catalog"}:
                if reviewed.get("pdf_page") is not None or not reviewed.get("reviewed_pages"):
                    raise ValueError("catalog category/absence needs reviewed pages, not a row page")
                if any(not 157 <= page <= 240 for page in reviewed["reviewed_pages"]):
                    raise ValueError("catalog category/absence review outside item chapter")
                if reviewed.get("semantic_status", "unreviewed") != "unreviewed":
                    raise ValueError("category/absence cannot claim a semantic field comparison")
            elif not 157 <= reviewed["pdf_page"] <= 240:
                raise ValueError("visual item override outside guide chapter")
            if reviewed.get("semantic_status", "").startswith("selected_fields") and not reviewed.get("checked_fields"):
                raise ValueError("selected-field comparison needs an explicit checked-field list")
            status = reviewed["status"]
        claims = value_claims(item["name"], page_spec, raw_pages)
        semantic_status = reviewed.get("semantic_status", "unreviewed") if reviewed else "unreviewed"
        if not reviewed and len(claims) == 1:
            semantic_status = "ocr_value_match" if claims[0]["value"] == item_values[item["id"]] else "ocr_value_disagree_unverified"
        visual_page = reviewed.get("pdf_page") if reviewed else None
        if status == "not_listed_in_printed_catalog":
            page_resolution = "visually_confirmed_not_listed_in_printed_catalog"
        elif status == "generic_category_only":
            page_resolution = "visually_confirmed_generic_category_only"
        elif visual_page is not None:
            page_resolution = "visual_entry_checked"
        elif status == "matched":
            page_resolution = "unique_exact_ocr_page_candidate"
        elif status == "ambiguous":
            page_resolution = "ambiguous_ocr_page_candidates"
        elif status == "absent":
            page_resolution = "ocr_absent_print_status_unknown"
        else:
            page_resolution = "retained_row_has_no_name"
        item_records.append({
            "id": item["id"],
            "name": item["name"],
            "retained_type": item["mm6_type"],
            "retained_category": item["mm6_category"],
            "candidate_guide_family": family,
            "candidate_prima_pdf_pages": page_spec,
            "catalog_match_status": status,
            "catalog_match_evidence": "visual_catalog_scan" if reviewed else "exact_normalized_ocr_name",
            "ocr_hit_pages_in_candidate_section": within,
            "ocr_hit_pages_elsewhere_in_item_chapter": outside,
            "guide_page_resolution_status": page_resolution,
            "guide_page_candidates": sorted(set(within + outside + ([visual_page] if visual_page else []))),
            "catalog_reviewed_pages": reviewed.get("reviewed_pages", []) if reviewed else [],
            "visual_review_page": visual_page,
            "visual_checked_fields": reviewed.get("checked_fields", []) if reviewed else [],
            "retained_base_value": item_values[item["id"]],
            "retained_field_schema": role_data["schema_cells"],
            "retained_field_schema_consumer": retained_schema_consumer(item, role),
            "retained_data_role": role,
            "retained_role_evidence": "retained item type, rarity and description marker only",
            "retained_quest_description_marker": role_data["quest_description_marker"],
            "retained_effect_token": spell_token if role in {"wand", "spell_scroll", "spellbook"} else None,
            "retained_spell_id": retained_spell_id,
            "source_wand_spell_id": source_wand_spell_id,
            "source_dispatch_status": source_dispatch_status,
            "source_dispatch_basis": source_dispatch_basis,
            "inherited_id_predicates": {
                "is_spellbook": 400 <= item_id <= 498,
                "is_artifact": 500 <= item_id <= 552,
                "is_spawnable_artifact": 500 <= item_id <= 528,
                "is_message_scroll": 700 <= item_id <= 781,
                "is_quest_item": 600 <= item_id <= 699,
            } if role != "outside_high_risk_scope" else None,
            "retained_event_held_item_references": held_item_refs.get(item_id, []) if item_id >= 400 else [],
            "ocr_value_claims": claims,
            "guide_semantic_status": semantic_status,
            "engine_consumer": consumer,
            "next_evidence": "guide page image + retained row + source consumer; F3 original confirmation; F4/F5 behavior",
        })
    monster_records = []
    for monster in monsters:
        hits = page_hits(monster["mm6_display_name"], pages, 35, 96)
        status = catalog_status(monster["mm6_display_name"], hits, [])
        reviewed = overrides.get("monsters", {}).get(str(monster["id"]))
        if reviewed:
            if reviewed["status"] not in {"matched", "ambiguous", "absent"}:
                raise ValueError("invalid visual monster override status")
            if not 35 <= reviewed["pdf_page"] <= 96:
                raise ValueError("visual monster override outside guide chapter")
            status = reviewed["status"]
        monster_records.append({
            "id": monster["id"],
            "name": monster["mm6_display_name"],
            "numeric_family": monster["numeric_family"],
            "numeric_tier": monster["numeric_tier"],
            "candidate_prima_pdf_pages": "35-96",
            "catalog_match_status": status,
            "catalog_match_evidence": "visual_scan" if reviewed else "exact_normalized_ocr_name",
            "ocr_hit_pages": hits,
            "visual_review_page": reviewed["pdf_page"] if reviewed else None,
            "guide_semantic_status": reviewed.get("semantic_status", "unreviewed") if reviewed else "unreviewed",
            "engine_consumer": "MonsterStats load; Actor/MonsterEnumFunctions/HostilityTable behavior requires title check",
            "next_evidence": "guide page image + retained row + source consumer; F3 original confirmation; F4/F5 encounter",
        })

    output = {
        "scope": "retained-probe-to-guide reconciliation queue; no original archive read or runtime claim",
        "source_report_hashes": {
            "item_range_input": item_report["input_sha256"],
            "combat_semantics": monster_report["sha256"],
            "prima_pdf": guide_report["sources"]["prima"]["pdf_sha256"],
            "prima_text": hashlib.sha256(prima_text_path.read_bytes()).hexdigest(),
            "retained_items_txt": hashlib.sha256(item_values_path.read_bytes()).hexdigest(),
            "retained_quest_graph_probe": quest_graph_hash,
            "current_item_source": source_fingerprints,
            "reviewed_overrides": hashlib.sha256(overrides_path.read_bytes()).hexdigest() if overrides else None,
        },
        "summary": {
            "item_ids": len(item_records),
            "monster_ids": len(monster_records),
            "item_candidate_families": dict(sorted(Counter(record["candidate_guide_family"] for record in item_records).items())),
            "item_catalog_status": dict(sorted(Counter(record["catalog_match_status"] for record in item_records).items())),
            "monster_catalog_status": dict(sorted(Counter(record["catalog_match_status"] for record in monster_records).items())),
            "visually_reviewed_item_ids": sum(record["visual_review_page"] is not None for record in item_records),
            "visually_confirmed_printed_catalog_absences": sum(record["guide_page_resolution_status"] == "visually_confirmed_not_listed_in_printed_catalog" for record in item_records),
            "visually_confirmed_generic_category_only": sum(record["guide_page_resolution_status"] == "visually_confirmed_generic_category_only" for record in item_records),
            "item_guide_page_resolution_status": dict(sorted(Counter(record["guide_page_resolution_status"] for record in item_records).items())),
            "visually_reviewed_monster_ids": sum(record["visual_review_page"] is not None for record in monster_records),
            "item_semantic_status": dict(sorted(Counter(record["guide_semantic_status"] for record in item_records).items())),
            "high_risk_item_ids": sum(record["retained_data_role"] != "outside_high_risk_scope" for record in item_records),
            "high_risk_retained_roles": dict(sorted(Counter(record["retained_data_role"] for record in item_records if record["retained_data_role"] != "outside_high_risk_scope").items())),
            "high_risk_source_dispatch_status": dict(sorted(Counter(record["source_dispatch_status"] for record in item_records if record["retained_data_role"] != "outside_high_risk_scope").items())),
            "special_band_held_item_ref_ids": sum(bool(record["retained_event_held_item_references"]) for record in item_records if record["id"] >= 400),
            "monster_semantic_status": dict(sorted(Counter(record["guide_semantic_status"] for record in monster_records).items())),
        },
        "items": item_records,
        "monsters": monster_records,
    }
    report_path.write_text(json.dumps(output, indent=2) + "\n")
    print(json.dumps(output["summary"], indent=2))


if __name__ == "__main__":
    main()
