#!/usr/bin/env python3
"""Index retained MM6 guide PDF pages without publishing their text.

This is a page-coverage check, not a semantic review or OCR validator. The
report belongs under the ignored owner-data tree. PDF page numbers are 1-based.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import logging
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REFERENCE = ROOT / "local-game-data/mm6/research-2026-11-05"
SOURCES = {
    "manual": ("mm6-manual.pdf", 61),
    "reference-card": ("mm6-reference-card.pdf", 5),
    "prima": ("mm6-prima-guide.pdf", 412),
}
# Prima's printed pagination is approximately PDF page +8 in these chapters.
# These ranges index the chapter families, not every claim inside them.
CATEGORIES = {
    "lore-and-story": {"prima": [(11, 22)]},
    "creation": {"manual": [(23, 26)], "reference-card": [(3, 3)], "prima": [(23, 34)]},
    "monsters": {"prima": [(35, 96)]},
    "input-and-screens": {"manual": [(17, 21), (30, 33)], "reference-card": [(2, 5)]},
    "progression": {"manual": [(26, 29), (40, 41)], "prima": [(241, 248), (311, 326)]},
    "combat-recovery-time": {"manual": [(37, 40)], "reference-card": [(4, 5)]},
    "spells": {"manual": [(42, 54)], "prima": [(97, 156)]},
    "items-and-alchemy": {"manual": [(27, 29), (33, 36)], "prima": [(157, 240)]},
    "economy-npcs-travel": {"manual": [(34, 37)], "prima": [(249, 280)]},
    "save-and-return": {"manual": [(55, 59)]},
    "essential-quests": {"prima": [(281, 310)]},
    "promotions-and-seer": {"prima": [(311, 326)]},
    "regional-quests": {"prima": [(327, 344)]},
    "map-appendix": {"prima": [(345, 412)]},
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_reader(reference: Path):
    # Local, ignored install is an option on machines without system pypdf.
    local_tools = reference / ".pdf-tools"
    if local_tools.is_dir():
        sys.path.insert(0, str(local_tools))
    try:
        from pypdf import PdfReader
    except ImportError as exc:
        raise SystemExit("Install pypdf locally to index guide pages") from exc
    logging.disable(logging.CRITICAL)  # damaged font maps can be noisy
    return PdfReader


def covered_pages(ranges: list[tuple[int, int]]) -> list[int]:
    return sorted({page for first, last in ranges for page in range(first, last + 1)})


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    reference = args.reference.resolve()
    report_path = (args.report or reference / "guide-coverage-probe.json").resolve()
    if not report_path.is_relative_to(ROOT / "local-game-data"):
        parser.error("report must stay under ignored local-game-data")

    PdfReader = load_reader(reference)
    sources = {}
    for name, (filename, expected_pages) in SOURCES.items():
        path = reference / filename
        if not path.is_file():
            raise SystemExit(f"missing retained PDF: {path}")
        reader = PdfReader(path)
        if len(reader.pages) != expected_pages:
            raise SystemExit(f"unexpected {name} page count: {len(reader.pages)}")
        pages = []
        for number, page in enumerate(reader.pages, 1):
            extracted = page.extract_text() or ""
            pages.append({
                "pdf_page": number,
                "text_chars": len(extracted),
                "text_sha256": hashlib.sha256(extracted.encode("utf-8")).hexdigest(),
                "needs_visual_check": len(extracted.strip()) < 100,
            })
        sources[name] = {
            "file": filename,
            "pdf_sha256": sha256(path),
            "page_count": len(pages),
            "pages": pages,
        }

    categories = {}
    for category, references in CATEGORIES.items():
        entries = {}
        for source, ranges in references.items():
            page_numbers = covered_pages(ranges)
            pages = sources[source]["pages"]
            entries[source] = {
                "pdf_page_ranges": ranges,
                "indexed_pages": len(page_numbers),
                "low_text_pages": [n for n in page_numbers if pages[n - 1]["needs_visual_check"]],
            }
        categories[category] = entries

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps({"sources": sources, "categories": categories}, indent=2) + "\n")
    for name, data in sources.items():
        low = sum(page["needs_visual_check"] for page in data["pages"])
        print(f"{name}: {data['page_count']} pages, {low} low-text pages")
    print(f"category families indexed: {len(categories)}")
    print(f"private report: {report_path}")


if __name__ == "__main__":
    main()
