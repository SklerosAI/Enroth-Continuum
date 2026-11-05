#!/usr/bin/env python3
"""Inventory an already extracted, private MM6 reference set.

The output contains proprietary filenames and table text. Keep both input and
output under ignored local-game-data; never commit or upload the output.
"""

import argparse
import csv
import hashlib
import json
from collections import Counter
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored extracted archive directory")
    parser.add_argument("output", type=Path, help="ignored catalog JSON path")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    output = args.output.resolve()
    if output == root or root not in output.parents:
        parser.error("output must be inside the private reference directory")

    catalog = {"format": 1, "archives": {}, "files": [], "tables": {}}
    for archive in ("icons", "Games", "BITMAPS", "SPRITES"):
        entries = root / f"{archive}.entries"
        if not entries.is_file() or not (root / archive).is_dir():
            parser.error(f"missing extracted archive or entry list: {archive}")
        catalog["archives"][archive] = {
            "entry_count": len(entries.read_bytes().splitlines()),
            "entry_list_sha256": sha256(entries),
        }

    counts = Counter()
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path == output:
            continue
        relative = path.relative_to(root).as_posix()
        suffix = path.suffix.lower()
        counts[suffix or "[none]"] += 1
        catalog["files"].append({
            "path": relative,
            "size": path.stat().st_size,
            "sha256": sha256(path),
        })
        if path.parent == root / "icons" and suffix == ".txt":
            raw = path.read_bytes().decode("cp1252", errors="replace")
            rows = []
            for number, cells in enumerate(csv.reader(raw.splitlines(), delimiter="\t"), 1):
                if any(cell.strip() for cell in cells):
                    rows.append({"line": number, "id": cells[0].strip() if cells else "", "cells": cells})
            catalog["tables"][path.name] = {"row_count": len(rows), "rows": rows}
    catalog["file_count"] = len(catalog["files"])
    catalog["file_types"] = dict(sorted(counts.items()))
    output.write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Cataloged {catalog['file_count']} files and {len(catalog['tables'])} text tables to {output}")


if __name__ == "__main__":
    main()
