#!/usr/bin/env python3
"""Build a private per-ID MM6 item/monster guide reconciliation queue.

This consumes retained, ignored probe reports only. Guide page ranges are
candidate chapter locators, not assertions that an individual row was checked.
"""

from __future__ import annotations

import argparse
import json
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
    if 400 <= item_id <= 499:
        return "artifact, relic or other special item (unresolved)", "212-231"
    if item_id >= 500:
        return "quest or miscellaneous item (unresolved)", "224-231"
    if kind in {"Weapon", "Weapon1or2", "Weapon2", "Missile"}:
        return "weapons", "159-180"
    if kind in {"Armor", "Shield", "Helm", "Boots", "Cloak", "cloak", "Belt", "Gauntlets", "Ring", "Amulet"}:
        return "bodily protection and jewelry", "180-200"
    return "unclassified item (requires direct guide/data join)", "157-240"


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
    items = item_report["items"]
    monsters = monster_report["records"]
    assert [item["id"] for item in items] == list(range(581))
    assert [monster["id"] for monster in monsters] == list(range(1, 174))
    assert guide_report["sources"]["prima"]["page_count"] == 412

    item_records = []
    for item in items:
        family, pages = guide_family(item)
        item_records.append({
            "id": item["id"],
            "name": item["name"],
            "retained_type": item["mm6_type"],
            "retained_category": item["mm6_category"],
            "candidate_guide_family": family,
            "candidate_prima_pdf_pages": pages,
            "guide_row_status": "not_directly_reconciled",
            "engine_consumer": "MM6ItemTable load; shared ItemTable/ItemEnumFunctions/UIPopup behavior requires title check",
            "next_evidence": "guide page image + retained row + source consumer; F3 original confirmation; F4/F5 behavior",
        })
    monster_records = []
    for monster in monsters:
        monster_records.append({
            "id": monster["id"],
            "name": monster["mm6_display_name"],
            "numeric_family": monster["numeric_family"],
            "numeric_tier": monster["numeric_tier"],
            "candidate_prima_pdf_pages": "35-96",
            "guide_row_status": "not_directly_reconciled",
            "engine_consumer": "MonsterStats load; Actor/MonsterEnumFunctions/HostilityTable behavior requires title check",
            "next_evidence": "guide page image + retained row + source consumer; F3 original confirmation; F4/F5 encounter",
        })

    output = {
        "scope": "retained-probe-to-guide reconciliation queue; no original archive read or runtime claim",
        "source_report_hashes": {
            "item_range_input": item_report["input_sha256"],
            "combat_semantics": monster_report["sha256"],
            "prima_pdf": guide_report["sources"]["prima"]["pdf_sha256"],
        },
        "summary": {
            "item_ids": len(item_records),
            "monster_ids": len(monster_records),
            "item_candidate_families": dict(sorted(Counter(record["candidate_guide_family"] for record in item_records).items())),
            "directly_reconciled_item_rows_in_this_queue": 0,
            "directly_reconciled_monster_rows_in_this_queue": 0,
        },
        "items": item_records,
        "monsters": monster_records,
    }
    report_path.write_text(json.dumps(output, indent=2) + "\n")
    print(json.dumps(output["summary"], indent=2))


if __name__ == "__main__":
    main()
