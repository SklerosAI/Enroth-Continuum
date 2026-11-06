#!/usr/bin/env python3
"""Inventory retained MM6 transport-house A/B/C fields without inferring gameplay.

Reads only an already retained, ignored text table. Exact route text is written
to an ignored local JSON report; stdout prints aggregate counts only.
"""

import argparse
import csv
import hashlib
import json
import re
from collections import Counter
from pathlib import Path


SOURCE_FILES = (
    "src/Engine/Tables/HouseTable.cpp",
    "src/GUI/UI/Houses/Transport.cpp",
)
DAY_TOKENS = {"M", "Tu", "T", "W", "Th", "F", "Sa", "Su", "Sun"}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def clean(cell: str) -> str:
    return cell.strip().strip('"').strip()


def classify_route(raw: str) -> dict:
    # Commas are part of the owner's table values; preserve the original cell.
    pieces = [part.strip().strip('"').strip() for part in raw.split(",")]
    if len(pieces) < 3 or not re.fullmatch(r"\d+", pieces[-1]):
        return {"kind": "opaque_special", "token_count": len(pieces)}
    days = pieces[1:-1]
    if not days or any(day not in DAY_TOKENS for day in days):
        return {"kind": "schedule_candidate_with_unknown_day", "token_count": len(pieces),
                "unknown_day_tokens": [day for day in days if day not in DAY_TOKENS]}
    return {"kind": "schedule_candidate", "token_count": len(pieces),
            "day_token_count": len(days), "duration_candidate": int(pieces[-1])}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="retained ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="JSON output inside that directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    output = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must stay inside ignored local-game-data/mm6")
    if not output.is_relative_to(root) or output.suffix.lower() != ".json" or not output.parent.is_dir():
        parser.error("report must be JSON inside the ignored reference directory")

    table = root / "icons/2devents.txt"
    if not table.is_file():
        parser.error("retained 2devents.txt missing")
    source_paths = {name: repo / name for name in SOURCE_FILES}
    if any(not path.is_file() for path in source_paths.values()):
        parser.error("tracked transport consumer source missing")
    rows = list(csv.reader(table.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t"))
    houses = []
    seen_ids = set()
    for line_number, row in enumerate(rows, 1):
        if not row or not row[0].strip().isdigit():
            continue
        if len(row) < 16:
            continue
        kind = clean(row[2])
        if kind not in ("Stables", "Boats"):
            continue
        house_id = int(row[0].strip())
        if house_id in seen_ids:
            raise ValueError(f"duplicate transport house ID {house_id}")
        seen_ids.add(house_id)
        routes = []
        for column in (13, 14, 15):
            raw_cell = row[column]
            value = clean(raw_cell)
            if not value or value == "-":
                continue
            routes.append({"column": column, "raw_cell": raw_cell,
                           "normalized": value, **classify_route(value)})
        houses.append({"house_id": house_id, "line": line_number, "kind": kind,
                       "source_map_cell": clean(row[3]), "value_cell": clean(row[12]),
                       "routes": routes})

    kind_counts = Counter(house["kind"] for house in houses)
    route_counts = Counter(route["kind"] for house in houses for route in house["routes"])
    slot_counts = Counter(str(route["column"]) for house in houses for route in house["routes"])
    summary = {"transport_houses": len(houses), "stable_houses": kind_counts["Stables"],
               "boat_houses": kind_counts["Boats"],
               "nonempty_abc_slots": sum(len(house["routes"]) for house in houses),
               "route_class_counts": dict(sorted(route_counts.items())),
               "slot_counts": dict(sorted(slot_counts.items()))}
    if summary["transport_houses"] != 21 or summary["stable_houses"] != 9 or summary["boat_houses"] != 12:
        raise ValueError("retained transport house inventory changed; review before writing report")
    result = {"format": 1, "scope": "retained MM6 table and current consumers; no original archive or runtime",
              "summary": summary, "houses": houses,
              "sha256": {"table": digest(table),
                         "sources": {name: digest(path) for name, path in source_paths.items()}}}
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
