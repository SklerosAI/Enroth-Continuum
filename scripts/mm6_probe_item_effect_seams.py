#!/usr/bin/env python3
"""Triage retained MM6 item-guide gaps against current static item-use paths.

Reads only ignored F2 reports and tracked source. Exact item identities and
event locators are written only to an ignored local JSON report.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def direct_use_ids(character_source: str, enum_source: str) -> dict[str, int]:
    start = character_source.index("    // Everything else\n")
    end = character_source.index("\nvoid Character::giveAward", start)
    branch = character_source[start:end]
    symbols = set(re.findall(r"pParty->pPickedItem\.itemId == (ITEM_[A-Z0-9_]+)", branch))
    values = {
        symbol: int(value)
        for symbol, value in re.findall(r"^\s*(ITEM_[A-Z0-9_]+)\s*=\s*(\d+)\s*,", enum_source, re.M)
    }
    missing = symbols - values.keys()
    if missing:
        raise ValueError(f"direct-use item enum values unresolved: {sorted(missing)}")
    return {symbol: values[symbol] for symbol in sorted(symbols)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    output = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must stay in ignored local-game-data/mm6")
    if not output.is_relative_to(root) or output.suffix.lower() != ".json" or not output.parent.is_dir():
        parser.error("report must be JSON inside the ignored reference directory")

    queue = root / "item-monster-guide-queue.json"
    data = json.loads(queue.read_text())
    rows = data["items"]
    if len(rows) != 581 or {row["id"] for row in rows} != set(range(581)):
        raise ValueError("expected each retained item ID 0..580 exactly once")
    unreviewed = [row for row in rows if row["guide_semantic_status"] == "unreviewed"]
    special = [row for row in rows if row["retained_data_role"] == "special_misc_role_unresolved"]
    if len(unreviewed) != 164 or len(special) != 67:
        raise ValueError("F2 queue status changed; review triage assumptions")

    char_path = repo / "src/Engine/Objects/Character.cpp"
    enum_path = repo / "src/Engine/Objects/ItemEnums.h"
    item_path = repo / "src/Engine/Objects/Item.cpp"
    predicate_path = repo / "src/Engine/Objects/ItemEnumFunctions.h"
    message_path = repo / "src/GUI/UI/UIMessageScroll.cpp"
    direct = direct_use_ids(char_path.read_text(), enum_path.read_text())
    if not direct or any(value <= 580 for value in direct.values()):
        raise ValueError("direct-use branch changed; inspect MM6 ID coverage")
    if "if (isQuestItem(itemId))" not in item_path.read_text() or "inline bool isQuestItem(ItemId item)" not in predicate_path.read_text():
        raise ValueError("quest-item sale predicate path changed")
    enum_source = enum_path.read_text()
    if "ITEM_MESSAGE_FROM_ERATHIA = 700," not in enum_source or "ITEM_781 = 781," not in enum_source:
        raise ValueError("inherited message-scroll range changed")
    if "if (!pGUIWindow_ScrollWindow && isMessageScroll(mscroll_id))" not in message_path.read_text():
        raise ValueError("message-scroll window gate changed")
    message_rows = [row for row in rows if row["retained_data_role"] == "message_scroll"]
    if len(message_rows) != 50 or any(not 500 <= row["id"] <= 549 for row in message_rows):
        raise ValueError("retained message-scroll ID band changed")

    def private_row(row: dict) -> dict:
        return {
            "id": row["id"],
            "name": row["name"],
            "retained_type": row["retained_type"],
            "retained_data_role": row["retained_data_role"],
            "candidate_guide_family": row["candidate_guide_family"],
            "guide_semantic_status": row["guide_semantic_status"],
            "catalog_match_status": row["catalog_match_status"],
            "visual_review_page": row["visual_review_page"],
            "retained_quest_description_marker": row["retained_quest_description_marker"],
            "held_item_event_operand_count": len(row["retained_event_held_item_references"]),
            "held_item_event_locators": row["retained_event_held_item_references"],
            "source_dispatch_status": row["source_dispatch_status"],
            "next_evidence": row["next_evidence"],
        }

    public = {
        "unreviewed_items": len(unreviewed),
        "unreviewed_by_retained_role": dict(sorted(Counter(row["retained_data_role"] for row in unreviewed).items())),
        "unreviewed_with_held_item_operand": sum(bool(row["retained_event_held_item_references"]) for row in unreviewed),
        "unreviewed_with_individual_printed_entry": sum(row["catalog_match_status"] == "matched" for row in unreviewed),
        "special_misc_role_unresolved": len(special),
        "special_misc_with_held_item_operand": sum(bool(row["retained_event_held_item_references"]) for row in special),
        "special_misc_with_individual_printed_entry": sum(row["catalog_match_status"] == "matched" for row in special),
        "special_misc_with_quest_description_marker": sum(bool(row["retained_quest_description_marker"]) for row in special),
        "special_misc_aliasing_inherited_spellbook_range": sum(bool(row["inherited_id_predicates"]["is_spellbook"]) for row in special),
        "special_misc_aliasing_inherited_quest_range": sum(bool(row["inherited_id_predicates"]["is_quest_item"]) for row in special),
        "current_misc_direct_use_symbol_count": len(direct),
        "current_misc_direct_use_symbols_within_mm6_item_range": sum(value <= 580 for value in direct.values()),
        "retained_mm6_message_scroll_ids": len(message_rows),
        "retained_message_scroll_ids_passing_inherited_window_gate": sum(700 <= row["id"] <= 781 for row in message_rows),
    }
    result = {
        "format": 1,
        "scope": "retained F2 item queue and tracked C++ only; no original install or MM6 launch",
        "summary": public,
        "unreviewed": [private_row(row) for row in unreviewed],
        "unresolved_special_misc": [private_row(row) for row in special],
        "current_misc_direct_use_enum_values": direct,
        "source_sha256": {str(path.relative_to(repo)): sha256(path) for path in (queue, char_path, enum_path, item_path, predicate_path, message_path)},
    }
    output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(public, sort_keys=True))


if __name__ == "__main__":
    main()
