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
        all_hits = page_hits(item["name"], pages, 157, 240)
        expected = candidate_pages(page_spec)
        within = [page for page in all_hits if page in expected]
        outside = [page for page in all_hits if page not in expected]
        status = catalog_status(item["name"], within, outside)
        reviewed = overrides.get("items", {}).get(str(item["id"]))
        if reviewed:
            if reviewed["status"] not in {"matched", "ambiguous", "absent"}:
                raise ValueError("invalid visual item override status")
            if not 157 <= reviewed["pdf_page"] <= 240:
                raise ValueError("visual item override outside guide chapter")
            status = reviewed["status"]
        claims = value_claims(item["name"], page_spec, raw_pages)
        semantic_status = reviewed.get("semantic_status", "unreviewed") if reviewed else "unreviewed"
        if not reviewed and len(claims) == 1:
            semantic_status = "ocr_value_match" if claims[0]["value"] == item_values[item["id"]] else "ocr_value_disagree_unverified"
        item_records.append({
            "id": item["id"],
            "name": item["name"],
            "retained_type": item["mm6_type"],
            "retained_category": item["mm6_category"],
            "candidate_guide_family": family,
            "candidate_prima_pdf_pages": page_spec,
            "catalog_match_status": status,
            "catalog_match_evidence": "visual_scan" if reviewed else "exact_normalized_ocr_name",
            "ocr_hit_pages_in_candidate_section": within,
            "ocr_hit_pages_elsewhere_in_item_chapter": outside,
            "visual_review_page": reviewed["pdf_page"] if reviewed else None,
            "retained_base_value": item_values[item["id"]],
            "ocr_value_claims": claims,
            "guide_semantic_status": semantic_status,
            "engine_consumer": "MM6ItemTable load; shared ItemTable/ItemEnumFunctions/UIPopup behavior requires title check",
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
            "reviewed_overrides": hashlib.sha256(overrides_path.read_bytes()).hexdigest() if overrides else None,
        },
        "summary": {
            "item_ids": len(item_records),
            "monster_ids": len(monster_records),
            "item_candidate_families": dict(sorted(Counter(record["candidate_guide_family"] for record in item_records).items())),
            "item_catalog_status": dict(sorted(Counter(record["catalog_match_status"] for record in item_records).items())),
            "monster_catalog_status": dict(sorted(Counter(record["catalog_match_status"] for record in monster_records).items())),
            "visually_reviewed_item_ids": sum(record["visual_review_page"] is not None for record in item_records),
            "visually_reviewed_monster_ids": sum(record["visual_review_page"] is not None for record in monster_records),
            "item_semantic_status": dict(sorted(Counter(record["guide_semantic_status"] for record in item_records).items())),
            "monster_semantic_status": dict(sorted(Counter(record["guide_semantic_status"] for record in monster_records).items())),
        },
        "items": item_records,
        "monsters": monster_records,
    }
    report_path.write_text(json.dumps(output, indent=2) + "\n")
    print(json.dumps(output["summary"], indent=2))


if __name__ == "__main__":
    main()
