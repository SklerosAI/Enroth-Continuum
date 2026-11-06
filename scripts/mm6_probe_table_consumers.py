#!/usr/bin/env python3
"""Match retained MM6 text tables to literal current C++ resource loads.

This is a narrow static index, not proof that a table is parsed correctly or
unused. The detailed report remains under the Git-ignored MM6 research tree.
"""

import argparse
import hashlib
import json
import re
from collections import defaultdict
from pathlib import Path


DIRECT_LOAD = re.compile(r'eventsData\s*\(\s*"([^"]+)"\s*\)', re.IGNORECASE)


def source_fingerprint(paths: list[Path], repo: Path) -> str:
    digest = hashlib.sha256()
    for path in paths:
        digest.update(path.relative_to(repo).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(hashlib.sha256(path.read_bytes()).digest())
    return digest.hexdigest()


def indexed_rows(catalog: dict, name: str) -> dict[int, tuple[str, ...]]:
    result = {}
    for row in catalog["tables"][name]["rows"]:
        if row["id"].isdigit():
            result[int(row["id"])] = tuple(cell.strip().casefold() for cell in row["cells"][1:])
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="retained ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="JSON report inside that directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if not root.is_relative_to(allowed) or root == allowed:
        parser.error("reference must be an ignored MM6 research directory")
    if not report.is_relative_to(root) or report.suffix.lower() != ".json":
        parser.error("JSON report must remain in the ignored research directory")
    if not report.parent.is_dir():
        parser.error("report parent directory does not exist")

    catalog_path = root / "catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    names = sorted(catalog["tables"])
    paths = sorted(path for path in (repo / "src").rglob("*")
                   if path.is_file() and path.suffix in {".cpp", ".h"})
    hits = defaultdict(list)
    for path in paths:
        for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            for match in DIRECT_LOAD.finditer(line):
                hits[match.group(1).casefold()].append({
                    "source": path.relative_to(repo).as_posix(), "line": number})

    rows = [{"table": name, "direct_literal_loads": hits[name.casefold()]}
            for name in names]
    autonote = indexed_rows(catalog, "autonote.txt")
    autonotes = indexed_rows(catalog, "autonotes.txt")
    common_note_ids = autonote.keys() & autonotes.keys()
    note_comparison = {
        "singular_numeric_rows": len(autonote),
        "plural_numeric_rows": len(autonotes),
        "shared_ids": len(common_note_ids),
        "shared_ids_with_identical_other_cells": sum(autonote[i] == autonotes[i] for i in common_note_ids),
        "singular_only_ids": sorted(autonote.keys() - autonotes.keys()),
        "plural_only_ids": sorted(autonotes.keys() - autonote.keys()),
        "shared_ids_with_differences": sorted(i for i in common_note_ids if autonote[i] != autonotes[i]),
    }
    result = {
        "format": 1,
        "scope": "retained table names against literal eventsData calls in current C++ only",
        "warning": "No literal load does not prove unused; a load does not prove format or semantic support.",
        "summary": {"tables": len(rows),
                    "with_literal_load": sum(bool(row["direct_literal_loads"]) for row in rows),
                    "without_literal_load": sum(not row["direct_literal_loads"] for row in rows)},
        "tables": rows,
        "autonote_plural_comparison": note_comparison,
        "catalog_sha256": hashlib.sha256(catalog_path.read_bytes()).hexdigest(),
        "cpp_source_fingerprint": source_fingerprint(paths, repo),
        "scanned_source_files": len(paths),
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result["summary"], indent=2))


if __name__ == "__main__":
    main()
