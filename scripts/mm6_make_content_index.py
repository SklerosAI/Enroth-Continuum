#!/usr/bin/env python3
"""Create private, reviewable MM6 content lists from the ignored catalog."""

import argparse
import csv
import json
from pathlib import Path


SCHOOLS = ("Fire", "Air", "Water", "Earth", "Spirit", "Mind", "Body", "Light", "Dark")


def cell(values: list[str], index: int) -> str:
    return values[index].strip() if index < len(values) else ""


def rows_for(catalog: dict, table: str):
    for row in catalog["tables"][table]["rows"]:
        if row["id"].isdigit():
            yield int(row["id"]), row["cells"], row["line"]


def write_tsv(path: Path, header: list[str], rows: list[list[object]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    catalog = json.loads((root / "catalog.json").read_text(encoding="utf-8"))

    specs = [
        ("items.txt", "item-index.tsv", ["id", "name", "equip_type", "skill", "icon", "source", "review_status"],
         lambda i, c, line: [i, cell(c, 2), cell(c, 4), cell(c, 5), cell(c, 1), f"items.txt:{line}",
                             "blank_template" if not cell(c, 2) else "unreviewed"]),
        ("spells.txt", "spell-index.tsv", ["id", "school", "name", "resistance", "source", "review_status"],
         lambda i, c, line: [i, SCHOOLS[(i - 1) // 11], cell(c, 2), cell(c, 3), f"spells.txt:{line}", "unreviewed"]),
        ("monsters.txt", "monster-index.tsv", ["id", "name", "level", "picture", "source", "review_status"],
         lambda i, c, line: [i, cell(c, 2), cell(c, 3), cell(c, 1), f"monsters.txt:{line}", "unreviewed"]),
        ("mapstats.txt", "map-index.tsv", ["id", "name", "file", "music_track", "source", "review_status"],
         lambda i, c, line: [i, cell(c, 1), cell(c, 2), cell(c, 25), f"mapstats.txt:{line}", "unreviewed"]),
        ("quests.txt", "quest-index.tsv", ["id", "quest_note", "data_note", "source", "review_status"],
         lambda i, c, line: [i, cell(c, 1), cell(c, 2), f"quests.txt:{line}", "unreviewed"]),
    ]
    for table, filename, header, transform in specs:
        rows = [transform(i, cells, line) for i, cells, line in rows_for(catalog, table)]
        ids = [row[0] for row in rows]
        if ids != list(range(min(ids), max(ids) + 1)):
            parser.error(f"{table} IDs are not a unique, continuous sequence")
        write_tsv(root / filename, header, rows)
        print(f"{filename}: {len(rows)} IDs, {ids[0]}–{ids[-1]}")


if __name__ == "__main__":
    main()
