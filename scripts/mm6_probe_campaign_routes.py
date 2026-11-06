#!/usr/bin/env python3
"""Join retained MM6 campaign references and current transport consumers.

F2 structural evidence only: this does not prove branch semantics, intended
travel schedules, campaign reachability, or live gameplay. Per-ID details and
source fingerprints are written only to an ignored local MM6 report.
"""

import argparse
import csv
import hashlib
import json
import re
import struct
from collections import Counter
from pathlib import Path


SOURCES = (
    "scripts/mm6_probe_campaign_routes.py",
    "src/Engine/Engine.cpp",
    "src/Engine/Tables/HouseTable.cpp",
    "src/Engine/Tables/TransitionTable.cpp",
    "src/Engine/Data/HouseEnums.h",
    "src/Engine/Data/HouseEnumFunctions.h",
    "src/GUI/UI/Houses/Transport.cpp",
    "src/Engine/Evt/EvtInstruction.cpp",
    "src/GUI/UI/NPCTopics.cpp",
    "src/Engine/Tables/NPCTable.cpp",
    "src/GUI/UI/UIHouses.cpp",
    "src/GUI/UI/UIGameOver.cpp",
    "src/Application/GameOver.cpp",
    "src/Application/Game.cpp",
)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def numeric_rows(path: Path) -> dict[int, list[str]]:
    rows = csv.reader(path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t")
    found = {}
    for row in rows:
        if row and row[0].strip().isdigit():
            row_id = int(row[0].strip())
            if row_id in found:
                raise ValueError(f"duplicate numeric ID in {path.name}")
            found[row_id] = row
    return found


def text(cell: str) -> str:
    return cell.strip().strip('"').strip()


def enum_value(source: str, name: str) -> int:
    match = re.search(r"\b" + re.escape(name) + r"\s*=\s*(\d+)\b", source)
    if not match:
        raise ValueError(f"missing expected HouseId enum value: {name}")
    return int(match.group(1))


def active_house_events(icons: Path, index: dict, houses: dict[int, list[str]]) -> tuple[list[dict], list[dict]]:
    references = []
    failures = []
    for entry in index["files"]:
        if entry["role"] not in ("listed_map", "global"):
            continue
        path = icons / entry["name"]
        data = path.read_bytes()
        if digest(path) != entry["sha256"]:
            failures.append({"kind": "stale_event_index", "file": path.name})
            continue
        for record in entry["records"]:
            if record["opcode"] != 2:
                continue
            offset, size = record["offset"], record["size"]
            if offset < 0 or size != 9 or offset + size > len(data) or data[offset] + 1 != size:
                failures.append({"kind": "bad_speak_house_shape", "file": path.name,
                                 "offset": offset, "size": size})
                continue
            house_id = struct.unpack_from("<I", data, offset + 5)[0]
            row = houses.get(house_id)
            references.append({"file": path.name, "event_id": record["event_id"],
                               "offset": offset, "house_id": house_id,
                               "house_present": row is not None,
                               "house_type": text(row[2]) if row and len(row) > 2 else None})
    return references, failures


def milestone_structure(graph: dict) -> dict:
    result = {}
    for name, candidate in graph["milestone_candidates"].items():
        candidate_bits = candidate["candidate_quest_rows_with_evt_bit_use"]
        reads = graph["quest_bit_read_events"]
        writes = graph["quest_bit_write_events"]
        read_events = {event for bit in candidate_bits for event in reads.get(str(bit), ())}
        write_events = {event for bit in candidate_bits for event in writes.get(str(bit), ())}
        result[name] = {
            "quest_row_candidates": candidate["quest_row_candidates"],
            "map_name_candidates": candidate["map_name_candidates"],
            "candidate_bits_with_evt_use": candidate_bits,
            "read_events": sorted(read_events),
            "write_events": sorted(write_events),
            "read_events_on_listed_maps": sorted(e for e in read_events if not e.startswith("global.evt:")),
            "write_events_on_listed_maps": sorted(e for e in write_events if not e.startswith("global.evt:")),
        }
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="retained ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="detailed JSON report inside that directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    output = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must stay inside ignored local-game-data/mm6")
    if not output.is_relative_to(root) or output.suffix.lower() != ".json" or not output.parent.is_dir():
        parser.error("report must be JSON inside the ignored research directory")

    paths = {
        "houses": root / "icons/2devents.txt",
        "transitions": root / "icons/trans.txt",
        "event_index": root / "event-probe.json",
        "dialogue": root / "dialogue-event-probe.json",
        "graph": root / "quest-graph-probe.json",
    }
    if any(not path.is_file() for path in paths.values()):
        parser.error("retained houses, transitions, event, dialogue and graph reports are required")
    source_paths = {name: repo / name for name in SOURCES}
    if any(not path.is_file() for path in source_paths.values()):
        parser.error("tracked campaign consumer source missing")

    houses = numeric_rows(paths["houses"])
    transitions = numeric_rows(paths["transitions"])
    event_index = json.loads(paths["event_index"].read_text(encoding="utf-8"))
    dialogue = json.loads(paths["dialogue"].read_text(encoding="utf-8"))
    graph = json.loads(paths["graph"].read_text(encoding="utf-8"))
    failures = []
    if graph.get("input_sha256", {}).get("icons/2devents.txt") != digest(paths["houses"]):
        failures.append({"kind": "stale_graph_house_table"})
    if graph.get("input_sha256", {}).get("event-probe.json") != digest(paths["event_index"]):
        failures.append({"kind": "stale_graph_event_index"})
    if graph.get("input_sha256", {}).get("dialogue-event-probe.json") != digest(paths["dialogue"]):
        failures.append({"kind": "stale_graph_dialogue_index"})

    enum_source = source_paths["src/Engine/Data/HouseEnums.h"].read_text(encoding="utf-8")
    transport_min = enum_value(enum_source, "HOUSE_STABLE_HARMONDALE")
    transport_max = enum_value(enum_source, "HOUSE_73")
    transport = []
    for house_id, row in houses.items():
        kind = text(row[2]) if len(row) > 2 else ""
        if kind in ("Stables", "Boats"):
            transport.append({"house_id": house_id, "type": kind,
                              "inside_inherited_route_array": transport_min <= house_id <= transport_max})
    references, house_failures = active_house_events(root / "icons", event_index, houses)
    failures.extend(house_failures)
    pseudo_endings = [ref for ref in references if ref["house_id"] in (600, 601)]
    milestones = milestone_structure(graph)
    missing_topics = [ref for ref in dialogue["npc_topic_references"]
                      if not ref["global_event_present"] and ref["current_handler"] == "generic_global_event"]
    teacher_global_topics = [ref for ref in dialogue["npc_topic_references"]
                             if ref["global_event_present"] and ref["current_handler"] == "teacher_special"]
    topic_summary = Counter(ref["event_id"] for ref in missing_topics)
    source_transport = source_paths["src/GUI/UI/Houses/Transport.cpp"].read_text(encoding="utf-8")
    source_transition = source_paths["src/Engine/Tables/TransitionTable.cpp"].read_text(encoding="utf-8")
    if "transportRoutes[houseId()]" not in source_transport or "std::array<TransportInfo, 35>" not in source_transport:
        failures.append({"kind": "transport_source_changed"})
    if "pTransitionStrings[i]" not in source_transition:
        failures.append({"kind": "transition_parser_changed"})

    summary = {
        "transport_houses": len(transport),
        "stable_houses": sum(x["type"] == "Stables" for x in transport),
        "boat_houses": sum(x["type"] == "Boats" for x in transport),
        "transport_houses_outside_inherited_route_array": sum(not x["inside_inherited_route_array"] for x in transport),
        "inherited_route_array_first": transport_min,
        "inherited_route_array_last": transport_max,
        "inherited_transport_schedule_rows": 35,
        "transition_description_numeric_rows": len(transitions),
        "active_speak_in_house_records": len(references),
        "active_speak_in_house_unresolved": sum(not x["house_present"] for x in references),
        "active_speak_in_pseudo_ending_house_records": len(pseudo_endings),
        "pseudo_ending_records_in_hive_script": sum(x["file"].casefold() == "hive.evt" for x in pseudo_endings),
        "active_speak_in_transport_house_records": sum(x["house_type"] in ("Stables", "Boats") for x in references),
        "npc_generic_missing_global_references": len(missing_topics),
        "npc_generic_missing_global_distinct_ids": len(topic_summary),
        "npc_teacher_range_with_global_references": len(teacher_global_topics),
        "npc_teacher_range_with_global_distinct_ids": len({ref["event_id"] for ref in teacher_global_topics}),
        "milestones": {name: {
            "quest_row_candidates": len(item["quest_row_candidates"]),
            "map_name_candidates": len(item["map_name_candidates"]),
            "candidate_bits_with_evt_use": len(item["candidate_bits_with_evt_use"]),
            "read_event_functions": len(item["read_events"]),
            "write_event_functions": len(item["write_events"]),
        } for name, item in milestones.items()},
        "structural_failures": len(failures),
    }
    result = {
        "format": 1,
        "scope": "retained MM6 references and tracked source only; original archives not reopened",
        "limits": "Keyword milestones and event-function co-occurrence are candidates, not branch or route proofs; no live travel tested.",
        "summary": summary,
        "transport_houses": sorted(transport, key=lambda x: x["house_id"]),
        "speak_in_house": references,
        "missing_generic_npc_topics": missing_topics,
        "teacher_range_topics_with_global_events": teacher_global_topics,
        "missing_generic_npc_topic_counts": dict(sorted(topic_summary.items())),
        "milestones": milestones,
        "failures": failures,
        "input_sha256": {name: digest(path) for name, path in paths.items()},
        "source_sha256": {name: digest(path) for name, path in source_paths.items()},
    }
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
