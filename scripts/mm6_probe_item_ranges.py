#!/usr/bin/env python3
"""Cross-check private MM6 item categories against inherited MM7 ID ranges.

The output report stays under the ignored reference directory. The source
range constants are from ItemEnums.h and are checked against its numeric enum
members before this script reports a comparison.
"""

import argparse
import csv
import hashlib
import json
import re
from collections import Counter
from pathlib import Path


MM7_RANGES = {
    "wand": ("ITEM_WAND_OF_FIRE", "ITEM_MYSTIC_WAND_OF_INCINERATION"),
    "spell_scroll": ("ITEM_SCROLL_TORCH_LIGHT", "ITEM_SCROLL_SOULDRINKER"),
    "spellbook": ("ITEM_SPELLBOOK_TORCH_LIGHT", "ITEM_SPELLBOOK_SOULDRINKER"),
    "artifact_and_special": ("ITEM_ARTIFACT_PUCK", "ITEM_SPECIAL_GROGNARDS_CUTLASS"),
    "message_scroll": ("ITEM_MESSAGE_FROM_ERATHIA", "ITEM_781"),
}
TYPE_NAMES = {"WeaponW": "wand", "Sscroll": "spell_scroll",
              "Book": "spellbook", "Mscroll": "message_scroll"}


def enum_members(path: Path) -> dict[str, int]:
    source = path.read_text(encoding="utf-8")
    return {name: int(value) for name, value in re.findall(r"\b(ITEM_[A-Z0-9_]+)\s*=\s*(\d+)\s*[,}]", source)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="output JSON inside that directory")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents:
        parser.error("report must remain in the private reference directory")
    items_path = root / "icons/items.txt"
    enum_path = Path(__file__).resolve().parents[1] / "src/Engine/Objects/ItemEnums.h"
    members = enum_members(enum_path)
    ranges = {name: (members[first], members[last])
              for name, (first, last) in MM7_RANGES.items()}
    rows = csv.reader(items_path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t")
    items = []
    for row in rows:
        if len(row) < 9 or not row[0].strip().isdigit():
            continue
        item_id = int(row[0])
        items.append({"id": item_id, "name": row[2], "mm6_type": row[4],
                      "mm6_category": TYPE_NAMES.get(row[4], "other"),
                      "rarity_field": row[8]})
    indexed = {item["id"]: item for item in items}
    if len(items) != len(indexed):
        parser.error("duplicate MM6 item IDs")

    catalog = []
    for name, (first, last) in ranges.items():
        matched = [item for item in items if first <= item["id"] <= last]
        catalog.append({"inherited_category": name, "first": first, "last": last,
                        "mm6_type_counts": dict(sorted(Counter(item["mm6_type"] for item in matched).items())),
                        "mm6_category_counts": dict(sorted(Counter(item["mm6_category"] for item in matched).items())),
                        "mm6_ids": [item["id"] for item in matched]})
    mm6_categories = {
        category: sorted(item["id"] for item in items if item["mm6_category"] == category)
        for category in TYPE_NAMES.values()
    }
    summary = {"item_rows": len(items), "lowest_id": min(indexed), "highest_id": max(indexed),
               "mm6_category_ranges": {category: {"count": len(ids), "first": ids[0], "last": ids[-1]}
                                       for category, ids in mm6_categories.items()},
               "inherited_ranges": {row["inherited_category"]: {"first": row["first"],
                                    "last": row["last"], "mm6_categories": row["mm6_category_counts"]}
                                    for row in catalog}}
    result = {"format": 1, "summary": summary, "ranges": catalog, "items": items,
              "input_sha256": {"items": hashlib.sha256(items_path.read_bytes()).hexdigest(),
                               "inherited_enum": hashlib.sha256(enum_path.read_bytes()).hexdigest()}}
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if not all(ids and ids == list(range(ids[0], ids[-1] + 1)) for ids in mm6_categories.values()):
        raise SystemExit("MM6 item categories are not contiguous; review before using range assumptions")


if __name__ == "__main__":
    main()
