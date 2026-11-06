#!/usr/bin/env python3
"""Join retained MM6 spell IDs to owned Prima guide headings, privately.

This establishes guide page/identity coverage, not effect correctness. The
per-ID result and any game-derived names must remain under ignored local data.
"""

from __future__ import annotations

import hashlib
import json
import logging
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / "local-game-data/mm6/research-2026-11-05"
GUIDE = REFERENCE / "mm6-prima-guide.pdf"
SPELL_REPORT = REFERENCE / "spell-effect-probe.json"
COVERAGE = REFERENCE / "guide-coverage-probe.json"
OUTPUT = REFERENCE / "spell-guide-claims-probe.json"

# PDF page numbers, verified from the start of each school in the retained scan.
SCHOOL_STARTS = (100, 106, 112, 118, 124, 131, 137, 143, 149)
HEADING = re.compile(r"^\s*(?:[lcs]\s*)?pell\s+([0-9IlO]+)\s*:\s*(.+)$", re.I)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def normalize(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.lower())


def heading_number(ocr: str) -> int:
    value = ocr.upper().replace("L", "I").replace("O", "0")
    if value == "II":
        return 11
    if value == "I0":
        return 10
    if value == "I":
        return 1
    return int(value)


def main() -> None:
    if not (REFERENCE / ".pdf-tools").is_dir():
        raise SystemExit("local private pypdf install is required")
    sys.path.insert(0, str(REFERENCE / ".pdf-tools"))
    from pypdf import PdfReader

    logging.disable(logging.CRITICAL)
    coverage = json.loads(COVERAGE.read_text())
    guide_sha = sha256(GUIDE)
    if guide_sha != coverage["sources"]["prima"]["pdf_sha256"]:
        raise SystemExit("retained Prima PDF differs from indexed guide")
    spell_data = json.loads(SPELL_REPORT.read_text())
    spells = spell_data["spells"]
    if len(spells) != 99:
        raise SystemExit("expected 99 retained spell rows")

    reader = PdfReader(GUIDE)
    if len(reader.pages) != 412:
        raise SystemExit("unexpected guide page count")
    headings: dict[int, dict] = {}
    for pdf_page in range(100, 156):
        school = max(index for index, start in enumerate(SCHOOL_STARTS)
                     if start <= pdf_page)
        page_text = reader.pages[pdf_page - 1].extract_text() or ""
        for line in page_text.splitlines():
            match = HEADING.match(line)
            if match is None:
                continue
            ordinal = heading_number(match.group(1))
            if not 1 <= ordinal <= 11:
                raise SystemExit(f"invalid spell ordinal on PDF p. {pdf_page}")
            spell_id = school * 11 + ordinal
            if spell_id in headings:
                raise SystemExit(f"duplicate guide spell ID {spell_id}")
            headings[spell_id] = {
                "pdf_page": pdf_page,
                "ocr_heading": match.group(2).strip(),
                "locator_method": "anchored OCR heading",
            }

    # The eleventh Water heading begins after a decorative glyph in the OCR.
    # Its page and printed name were checked in the scan; preserve this exception.
    if 33 not in headings:
        headings[33] = {
            "pdf_page": 117,
            "ocr_heading": "Lloyd's Beacon",
            "locator_method": "manual scan/OCR exception",
        }

    if set(headings) != set(range(1, 100)):
        raise SystemExit(f"guide heading gaps: {sorted(set(range(1, 100)) - set(headings))}")

    records = []
    mismatches = []
    for spell in spells:
        spell_id = spell["spell_id"]
        heading = headings[spell_id]
        source_name = spell["retained"]["name"]
        guide_name = heading["ocr_heading"]
        exact = normalize(source_name) == normalize(guide_name)
        ocr_exception = spell_id == 42 and normalize(guide_name) == "tumtostone"
        if not exact and not ocr_exception:
            mismatches.append(spell_id)
        records.append({
            "spell_id": spell_id,
            "school_index": (spell_id - 1) // 11,
            "retained_name": source_name,
            **heading,
            "identity_match": "normalized" if exact else "scan-checked OCR variant" if ocr_exception else "unresolved",
            "retained_mana_normal_expert_master": spell["retained"]["mana_normal_expert_master"],
            "inherited_mana_normal_expert_master": spell["current_spell_data"]["mana"][:3],
            "current_case_lines": spell["current_cast_case_lines"],
            "inherited_label_status": spell["label_status"],
            "guide_effect_status": "pending direct claim-level scan and source comparison",
        })
    if mismatches:
        raise SystemExit(f"guide/source identity disagreements: {mismatches}")

    report = {
        "scope": "Retained extracted guide and spell report only; no original archive or live game access",
        "warning": "All 99 heading locators are mapped; cost, target, effect, mastery and recovery are not validated for all IDs.",
        "guide_sha256": guide_sha,
        "spell_report_sha256": sha256(SPELL_REPORT),
        "summary": {
            "spell_ids": len(records),
            "guide_heading_normalized_matches": sum(r["identity_match"] == "normalized" for r in records),
            "guide_heading_scan_checked_ocr_variants": sum(r["identity_match"] == "scan-checked OCR variant" for r in records),
            "school_start_pdf_pages": list(SCHOOL_STARTS),
            "claim_level_effect_reviews_complete": 0,
        },
        "records": records,
    }
    if not OUTPUT.resolve().is_relative_to(ROOT / "local-game-data"):
        raise SystemExit("private report path escaped ignored local data")
    OUTPUT.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report["summary"], indent=2))
    print(f"private report: {OUTPUT}")


if __name__ == "__main__":
    main()
