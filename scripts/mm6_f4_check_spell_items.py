#!/usr/bin/env python3
"""Check the private, F3-matched MM6 spell-item bands without printing rows."""

import argparse
import json
from pathlib import Path


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def check(research_dir: Path):
    spell = read_json(research_dir / "spell-effect-probe.json")
    items = read_json(research_dir / "item-range-probe.json")
    f3 = read_json(research_dir / "F3_CONTENT_RECONCILIATION_2026-11-05.json")

    assert not f3["problems"]
    assert items["input_sha256"]["items"] == spell["input_sha256"]["items"]
    by_name = {row["name"].casefold(): row for row in f3["files"]}
    for name, source_key in (("items.txt", "items"), ("spells.txt", "spells")):
        row = by_name[name]
        assert row["retained_sha256"] == spell["input_sha256"][source_key]
        assert row["decoded_matches_retained"] and row["retained_matches_f2_catalog"]

    item_rows = {row["id"]: row for row in items["items"]}
    assert len(spell["spells"]) == 99
    for spell_id, row in enumerate(spell["spells"], start=1):
        scroll_id, book_id = 199 + spell_id, 299 + spell_id
        assert row["spell_id"] == spell_id
        assert row["candidate_scroll_item_id"] == scroll_id
        assert row["candidate_spellbook_item_id"] == book_id
        assert item_rows[scroll_id]["mm6_category"] == "spell_scroll"
        assert item_rows[book_id]["mm6_category"] == "spellbook"
        assert item_rows[scroll_id]["name"] == row["scroll_item_name"]
        assert item_rows[book_id]["name"] == row["spellbook_item_name"]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research_dir", type=Path, help="Ignored MM6 research directory, never the original installation")
    args = parser.parse_args()
    check(args.research_dir)
    print("Verified 99 MM6 scroll and 99 book item joins against F3-matched retained tables.")
