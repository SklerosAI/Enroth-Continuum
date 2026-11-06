#!/usr/bin/env python3
"""Audit retained MM6 text-table shapes and selected cross-table ID edges.

This F2 probe reads only the ignored retained copy. It records detailed IDs,
row hashes, and source locations in an ignored report; stdout is aggregate.
Shape and reference integrity do not prove gameplay semantics or reachability.
"""

import argparse
import csv
import hashlib
import json
import re
from collections import Counter
from pathlib import Path


TABLES = (
    "2devents.txt", "autonote.txt", "autonotes.txt", "awards.txt",
    "class.txt", "errorlog.txt", "global.txt", "items.txt",
    "mapstats.txt", "merchant.txt", "monsters.txt", "npcbtb.txt",
    "npcdata.txt", "npcnames.txt", "npcnews.txt", "npcprof.txt",
    "npctext.txt", "npctopic.txt", "passwords.txt", "proftext.txt",
    "quests.txt", "rnditems.txt", "scroll.txt", "skilldes.txt",
    "spcitems.txt", "spells.txt", "stats.txt", "stditems.txt",
    "trans.txt", "useitems.txt",
)
NO_LITERAL_LOAD = {
    "autonotes.txt", "errorlog.txt", "npcbtb.txt",
    "passwords.txt", "proftext.txt", "useitems.txt",
}
SOURCE_FILES = (
    "src/Engine/Engine.cpp", "src/Engine/Localization.cpp",
    "src/Engine/Tables/AutonoteTable.cpp", "src/Engine/Tables/AwardTable.cpp",
    "src/Engine/Tables/HouseTable.cpp", "src/Engine/Tables/MM6ItemTable.cpp",
    "src/Engine/Tables/MapTable.cpp", "src/Engine/Tables/MerchantTable.cpp",
    "src/Engine/Tables/MessageScrollTable.cpp", "src/Engine/Tables/NPCTable.cpp",
    "src/Engine/Tables/QuestTable.cpp", "src/Engine/Tables/TransitionTable.cpp",
    "src/Engine/Objects/Monsters.cpp", "src/Engine/Spells/Spells.cpp",
    "src/GUI/UI/UIHouses.cpp", "src/GUI/UI/NPCTopics.cpp",
    "scripts/mm6_probe_table_semantics.py",
)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tab_rows(path: Path) -> list[list[str]]:
    return list(csv.reader(path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t"))


def numeric(rows: list[list[str]]) -> dict[int, list[str]]:
    result = {}
    for row in rows:
        if row and row[0].strip().isdigit():
            row_id = int(row[0].strip())
            if row_id in result:
                raise ValueError("duplicate numeric ID in retained table")
            result[row_id] = row
    return result


def positive_refs(rows: list[list[str]], column: int) -> list[int]:
    result = []
    for row in rows:
        if len(row) > column and row[column].strip().isdigit():
            value = int(row[column])
            if value > 0:
                result.append(value)
    return result


def populated(rows: list[list[str]], column: int, zero_is_blank: bool = False) -> int:
    values = (row[column].strip().strip('"') if len(row) > column else "" for row in rows)
    return sum(bool(value) and (not zero_is_blank or value != "0") for value in values)


def edge(name: str, refs: list[int], target: set[int]) -> dict:
    missing = sorted(set(refs) - target)
    return {"name": name, "reference_count": len(refs),
            "distinct_reference_ids": len(set(refs)),
            "missing_reference_count": sum(value not in target for value in refs),
            "missing_distinct_ids": missing}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="JSON report inside that directory")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must be an ignored MM6 research directory")
    if not report.is_relative_to(root) or report.suffix.lower() != ".json" or not report.parent.is_dir():
        parser.error("JSON report must stay in the ignored reference directory")
    paths = {name: root / "icons" / name for name in TABLES}
    sources = {name: repo / name for name in SOURCE_FILES}
    missing = [str(path) for path in list(paths.values()) + list(sources.values()) if not path.is_file()]
    if missing:
        parser.error("missing retained input or tracked source: " + ", ".join(missing))

    tables = {name: tab_rows(path) for name, path in paths.items()}
    numeric_rows = {name: numeric(rows) for name, rows in tables.items()}
    table_shapes = {}
    for name, lines in tables.items():
        filled = [row for row in lines if any(cell.strip() for cell in row)]
        indexed = numeric_rows[name]
        table_shapes[name] = {
            "physical_lines": len(lines), "nonempty_lines": len(filled),
            "numeric_rows": len(indexed), "numeric_ids": sorted(indexed),
            "nonempty_row_widths": dict(sorted(Counter(len(row) for row in filled).items())),
            "numeric_row_widths": dict(sorted(Counter(len(row) for row in indexed.values()).items())),
            "nonnumeric_nonempty_rows": len(filled) - len(indexed),
            "numeric_row_hashes": {str(row_id): hashlib.sha256("\t".join(row).encode("utf-8")).hexdigest()
                                   for row_id, row in indexed.items()},
        }

    loader_sites = {name: [] for name in TABLES}
    pattern = re.compile(r'eventsData\("([^"]+\.txt)"\)')
    for source_name, path in sources.items():
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            for match in pattern.finditer(line):
                key = match.group(1).casefold()
                if key in loader_sites:
                    loader_sites[key].append({"source": source_name, "line": line_number})
    observed_no_load = {name for name, sites in loader_sites.items() if not sites}
    if observed_no_load != NO_LITERAL_LOAD:
        raise ValueError("literal loader inventory changed; review schema mapping")

    ids = {name: set(rows) for name, rows in numeric_rows.items()}
    npc = list(numeric_rows["npcdata.txt"].values())
    houses = list(numeric_rows["2devents.txt"].values())
    joins = [
        edge("npcdata.house_to_2devents", positive_refs(npc, 6), ids["2devents.txt"]),
        edge("npcdata.profession_to_npcprof", positive_refs(npc, 7), ids["npcprof.txt"]),
        edge("npcdata.topic1_to_npctopic", positive_refs(npc, 10), ids["npctopic.txt"]),
        edge("npcdata.topic2_to_npctopic", positive_refs(npc, 11), ids["npctopic.txt"]),
        edge("npcdata.topic3_to_npctopic", positive_refs(npc, 12), ids["npctopic.txt"]),
        edge("2devents.exit_map_to_mapstats", positive_refs(houses, 21), ids["mapstats.txt"]),
        edge("rnditems.item_to_items", list(ids["rnditems.txt"]), ids["items.txt"]),
        edge("proftext.profession_to_npcprof", list(ids["proftext.txt"]), ids["npcprof.txt"]),
        edge("scroll.row_to_items", list(ids["scroll.txt"]), ids["items.txt"]),
    ]
    scroll_message_ids = [value for value in ids["scroll.txt"] if 500 <= value <= 549]
    message_edge = edge("scroll.MM6_message_range_to_items", scroll_message_ids, ids["items.txt"])
    joins.append(message_edge)
    award_rows = list(numeric_rows["awards.txt"].values())
    award_rows_with_priority = populated(award_rows, 2)
    if award_rows_with_priority != 0:
        raise ValueError("MM6 award priority column changed; review current parser skip")
    summary = {
        "retained_text_tables": len(TABLES),
        "tables_with_direct_literal_loader": len(TABLES) - len(observed_no_load),
        "tables_without_direct_literal_loader": len(observed_no_load),
        "numeric_id_tables": sum(bool(rows) for rows in numeric_rows.values()),
        "cross_table_edges_checked": len(joins),
        "cross_table_edges_with_missing_targets": sum(bool(join["missing_distinct_ids"]) for join in joins),
        "npc_house_refs": joins[0]["reference_count"],
        "npc_house_missing": joins[0]["missing_reference_count"],
        "npc_profession_refs": joins[1]["reference_count"],
        "npc_profession_missing": joins[1]["missing_reference_count"],
        "npc_topic_refs": sum(join["reference_count"] for join in joins[2:5]),
        "npc_topic_missing": sum(join["missing_reference_count"] for join in joins[2:5]),
        "house_exit_map_refs": joins[5]["reference_count"],
        "house_exit_map_missing": joins[5]["missing_reference_count"],
        "message_scroll_rows_in_loader_range": len(scroll_message_ids),
        "message_scroll_missing_items": message_edge["missing_reference_count"],
        "all_scroll_rows_missing_items": joins[8]["missing_reference_count"],
        "award_numeric_rows": len(award_rows),
        "award_rows_with_priority_cell": award_rows_with_priority,
        "award_rows_skipped_by_current_parser": len(award_rows) - award_rows_with_priority,
        "quest_rows_with_visible_text": populated(list(numeric_rows["quests.txt"].values()), 1, True),
        "npc_topic_rows_with_label": populated(list(numeric_rows["npctopic.txt"].values()), 1, True),
        "npc_text_rows_with_text": populated(list(numeric_rows["npctext.txt"].values()), 1, True),
        "transition_rows_with_description": populated(list(numeric_rows["trans.txt"].values()), 1, True),
        "item_rows_with_name": populated(list(numeric_rows["items.txt"].values()), 2, True),
    }
    payload = {
        "format": 1,
        "scope": "retained text-table shapes, literal loader sites, selected numeric joins; no archive or runtime opened",
        "warning": "Reference presence does not prove correct MM6 parsing, semantics, selection or reachability.",
        "summary": summary,
        "table_shapes": table_shapes,
        "literal_loader_sites": loader_sites,
        "cross_table_edges": joins,
        "input_sha256": {name: digest(path) for name, path in paths.items()},
        "source_sha256": {name: digest(path) for name, path in sources.items()},
    }
    report.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
