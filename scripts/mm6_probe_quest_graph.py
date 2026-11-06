#!/usr/bin/env python3
"""Build a private structural MM6 campaign graph from retained references.

This is an F2 evidence graph, not a proof of quest semantics or reachability.
Only aggregate counts go to stdout. The detailed graph must remain beneath
ignored local-game-data/mm6/, beside the retained extracted references.
"""

import argparse
import csv
import hashlib
import json
import re
import struct
from collections import Counter, defaultdict
from pathlib import Path


# Words from the public campaign milestones in project/MM6_GUIDE_RECONCILIATION.md.
# Matches are text candidates only; a matching quest row is not proof that its
# numeric row ID is an EVT quest bit or that the milestone is playable.
MILESTONE_TERMS = {
    "opening": ("sorpigal", "goblinwatch"),
    "council": ("high council", "nicolai", "traitor"),
    "oracle": ("oracle", "crystal"),
    "varn_control": ("varn", "control center"),
    "hive_ending": ("archibald", "hive", "void"),
    "promotions": ("cavalier", "priest", "wizard", "crusader", "battle mage", "druid"),
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def text_rows(path: Path) -> list[list[str]]:
    raw = path.read_bytes().decode("cp1252", errors="replace")
    return list(csv.reader(raw.splitlines(), delimiter="\t"))


def numeric_rows(path: Path) -> list[list[str]]:
    return [row for row in text_rows(path) if row and row[0].strip().isdigit()]


def clean(token: str) -> str:
    return token.strip().strip('"').strip()


def event_key(filename: str, event_id: int) -> str:
    return f"{filename.casefold()}:{event_id}"


def parse_events(icons: Path, event_index: dict, map_by_stem: dict[str, int]):
    events = {}
    links = []
    issues = []
    quest_reads = defaultdict(set)
    quest_writes = defaultdict(set)
    input_hashes = {}
    counts = Counter()

    for entry in event_index["files"]:
        if entry["role"] not in ("listed_map", "global"):
            continue
        path = icons / entry["name"]
        data = path.read_bytes()
        input_hashes[path.name] = sha256(path)
        if input_hashes[path.name] != entry["sha256"]:
            issues.append({"kind": "stale_event_index", "file": path.name})
            continue
        origin_stem = path.stem.casefold()
        origin_map = map_by_stem.get(origin_stem)
        for record in entry["records"]:
            offset, size = record["offset"], record["size"]
            if offset < 0 or size < 5 or offset + size > len(data) or data[offset] + 1 != size:
                issues.append({"kind": "bad_event_frame", "file": path.name, "offset": offset})
                continue
            event_id, opcode = record["event_id"], record["opcode"]
            key = event_key(path.name, event_id)
            node = events.setdefault(key, {"file": path.name, "event_id": event_id,
                                           "origin_map_id": origin_map, "opcodes": []})
            node["opcodes"].append(opcode)
            payload = data[offset + 5:offset + size]
            counts[opcode] += 1

            if opcode in (14, 16, 17, 18):
                expected = 6 if opcode == 14 else 5
                if len(payload) != expected:
                    issues.append({"kind": "bad_variable_shape", "file": path.name, "offset": offset})
                    continue
                var_id = payload[0]
                value = struct.unpack_from("<I", payload, 1)[0]
                links.append({"kind": "event_variable_operand", "event": key,
                              "variable_id": var_id, "operand": value,
                              "opcode": opcode, "offset": offset})
                if var_id == 16:  # MM6 quest-bit variable; bit semantics still require F2 review.
                    edge = {"event": key, "bit": value, "opcode": opcode, "offset": offset}
                    if opcode == 14:
                        quest_reads[value].add(key)
                        links.append({"kind": "quest_bit_read", **edge})
                    else:
                        quest_writes[value].add(key)
                        links.append({"kind": "quest_bit_write", **edge})
            elif opcode == 6:  # MoveToMap: six dwords, house/picture bytes, NUL name.
                if len(payload) < 27 or payload[-1] != 0:
                    issues.append({"kind": "bad_move_shape", "file": path.name, "offset": offset})
                    continue
                destination = payload[26:-1].decode("cp1252", errors="replace")
                stem = Path(destination).stem.casefold()
                if not stem or stem.startswith("0"):
                    links.append({"kind": "script_stay_on_map", "event": key, "offset": offset})
                else:
                    target = map_by_stem.get(stem)
                    links.append({"kind": "script_move_to_map", "event": key,
                                  "target_map_id": target, "target_name": destination,
                                  "offset": offset})
                    if target is None:
                        issues.append({"kind": "unresolved_move_target", "file": path.name,
                                       "offset": offset, "target_name": destination})
            elif opcode == 39 and len(payload) == 9:  # SetNPCTopic
                npc_id, slot, target_event = struct.unpack_from("<IBI", payload)
                links.append({"kind": "event_sets_npc_topic", "event": key,
                              "npc_id": npc_id, "slot": slot,
                              "target_global_event_id": target_event, "offset": offset})
            elif opcode == 40 and len(payload) == 8:  # MoveNPC
                npc_id, house_id = struct.unpack_from("<II", payload)
                links.append({"kind": "event_moves_npc", "event": key,
                              "npc_id": npc_id, "house_id": house_id, "offset": offset})
            elif opcode == 41 and len(payload) == 6:  # GiveItem
                item_id = struct.unpack_from("<I", payload, 2)[0]
                links.append({"kind": "event_gives_item", "event": key,
                              "item_id": item_id, "offset": offset})
            elif opcode == 42 and len(payload) == 4:  # ChangeEvent
                changed_event = struct.unpack_from("<I", payload)[0]
                links.append({"kind": "event_changes_event", "event": key,
                              "changed_event_id": changed_event, "offset": offset})

    return events, links, issues, quest_reads, quest_writes, input_hashes, counts


def house_links(houses: list[list[str]], map_by_stem: dict[str, int], map_ids: set[int]):
    links = []
    issues = []
    for row in houses:
        house_id = int(row[0].strip())
        origin_token = clean(row[3]).casefold() if len(row) > 3 else ""
        if re.fullmatch(r"[a-e][1-3]", origin_token):
            origin = map_by_stem.get("out" + origin_token)
            links.append({"kind": "house_origin_map_candidate", "house_id": house_id,
                          "map_id": origin, "source_token": origin_token})
            if origin is None:
                issues.append({"kind": "unresolved_house_origin", "house_id": house_id})
        elif origin_token:
            links.append({"kind": "house_origin_unresolved", "house_id": house_id,
                          "source_token": origin_token})
        raw_exit = clean(row[21]) if len(row) > 21 else ""
        if raw_exit.lstrip("-").isdigit() and int(raw_exit):
            destination = int(raw_exit)
            raw_gate = clean(row[22]) if len(row) > 22 else ""
            links.append({"kind": "house_exit_map", "house_id": house_id,
                          "target_map_id": destination,
                          "restriction_candidate": int(raw_gate) if raw_gate.lstrip("-").isdigit() else None})
            if destination not in map_ids:
                issues.append({"kind": "unresolved_house_exit", "house_id": house_id,
                               "target_map_id": destination})
    return links, issues


def milestones(quest_rows: list[list[str]], map_rows: list[list[str]],
               quest_reads: dict, quest_writes: dict):
    result = {}
    for label, terms in MILESTONE_TERMS.items():
        quest_candidates = sorted({int(row[0].strip()) for row in quest_rows
                                   if any(term in " ".join(row[1:]).casefold() for term in terms)})
        map_candidates = sorted({int(row[0].strip()) for row in map_rows
                                 if any(term in clean(row[1]).casefold() for term in terms)})
        result[label] = {
            "method": "keyword match to private quest-note or map name; candidate only",
            "terms": terms,
            "quest_row_candidates": quest_candidates,
            "map_name_candidates": map_candidates,
            "candidate_quest_rows_with_evt_bit_use": [value for value in quest_candidates
                                                       if value in quest_reads or value in quest_writes],
            "candidate_quest_rows_with_read_and_write": [value for value in quest_candidates
                                                          if value in quest_reads and value in quest_writes],
        }
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must be an ignored MM6 research directory")
    if not report.is_relative_to(root) or report.suffix.lower() != ".json":
        parser.error("report must be JSON inside the ignored research directory")
    icons = root / "icons"
    names = ("mapstats.txt", "2devents.txt", "npcdata.txt", "npctopic.txt", "quests.txt", "global.evt")
    required = [icons / name for name in names] + [root / "event-probe.json", root / "dialogue-event-probe.json"]
    missing = [str(p) for p in required if not p.is_file()]
    if missing:
        parser.error("missing retained references: " + ", ".join(missing))

    map_rows = numeric_rows(icons / "mapstats.txt")
    house_rows = numeric_rows(icons / "2devents.txt")
    quest_rows = numeric_rows(icons / "quests.txt")
    map_by_stem = {Path(clean(row[2])).stem.casefold(): int(row[0].strip()) for row in map_rows}
    map_ids = set(map_by_stem.values())
    event_index = json.loads((root / "event-probe.json").read_text(encoding="utf-8"))
    dialogue = json.loads((root / "dialogue-event-probe.json").read_text(encoding="utf-8"))
    events, links, issues, reads, writes, event_hashes, opcodes = parse_events(icons, event_index, map_by_stem)
    for filename in ("mapstats.txt", "npcdata.txt", "npctopic.txt"):
        if dialogue.get("input_sha256", {}).get(filename) != sha256(icons / filename):
            issues.append({"kind": "stale_dialogue_index", "file": filename})
    for filename, digest in event_hashes.items():
        if dialogue.get("input_sha256", {}).get("events", {}).get(filename) != digest:
            issues.append({"kind": "stale_dialogue_index", "file": filename})
    house_edges, house_issues = house_links(house_rows, map_by_stem, map_ids)
    links.extend(house_edges)
    issues.extend(house_issues)
    for ref in dialogue["npc_topic_references"]:
        links.append({"kind": "npc_topic_reference", "npc_id": ref["npc_id"],
                      "slot": ref["slot"], "target_global_event_id": ref["event_id"],
                      "global_event_present": ref["global_event_present"],
                      "current_handler": ref["current_handler"]})

    candidate_milestones = milestones(quest_rows, map_rows, reads, writes)
    quest_bits = set(reads) | set(writes)
    global_event_ids = {node["event_id"] for node in events.values() if node["file"].casefold() == "global.evt"}
    dynamic_topics = [x for x in links if x["kind"] == "event_sets_npc_topic"]
    summary = {
        "maps": len(map_rows), "active_evt_files": len(event_hashes),
        "event_functions": len(events),
        "script_move_targets": sum(x["kind"] == "script_move_to_map" for x in links),
        "script_move_targets_resolved": sum(x["kind"] == "script_move_to_map" and x["target_map_id"] is not None for x in links),
        "script_stay_on_map": sum(x["kind"] == "script_stay_on_map" for x in links),
        "house_origin_candidates": sum(x["kind"] == "house_origin_map_candidate" for x in links),
        "house_origins_unresolved": sum(x["kind"] == "house_origin_unresolved" for x in links),
        "house_exit_map_links": sum(x["kind"] == "house_exit_map" for x in links),
        "quest_bit_ids": len(quest_bits),
        "variable_operands": sum(x["kind"] == "event_variable_operand" for x in links),
        "distinct_variable_ids": len({x["variable_id"] for x in links
                                      if x["kind"] == "event_variable_operand"}),
        "quest_bit_ids_read": len(reads), "quest_bit_ids_written": len(writes),
        "quest_bit_ids_read_and_written": len(set(reads) & set(writes)),
        "quest_bit_ids_read_without_script_write": len(set(reads) - set(writes)),
        "quest_bit_ids_written_without_script_read": len(set(writes) - set(reads)),
        "npc_topic_references": sum(x["kind"] == "npc_topic_reference" for x in links),
        "event_set_npc_topic": opcodes[39], "event_move_npc": opcodes[40],
        "dynamic_nonzero_topics_without_global_event": sum(
            x["target_global_event_id"] != 0 and x["target_global_event_id"] not in global_event_ids
            for x in dynamic_topics),
        "event_give_item": opcodes[41], "event_change_event": opcodes[42],
        "milestone_candidates": {name: {"quest_rows": len(x["quest_row_candidates"]),
                                        "maps": len(x["map_name_candidates"]),
                                        "quest_rows_with_evt_bit_use": len(x["candidate_quest_rows_with_evt_bit_use"]),
                                        "quest_rows_with_read_and_write": len(x["candidate_quest_rows_with_read_and_write"])}
                                 for name, x in candidate_milestones.items()},
        "structural_issues": len(issues),
    }
    result = {
        "format": 1,
        "scope": "retained MM6 references only; no original archive read",
        "confidence": "structural links and keyword candidates, not executable quest reachability",
        "summary": summary,
        "maps": [{"id": int(row[0].strip()), "name": clean(row[1]), "file": clean(row[2])} for row in map_rows],
        "events": events,
        "links": links,
        "quest_bit_read_events": {str(k): sorted(v) for k, v in sorted(reads.items())},
        "quest_bit_write_events": {str(k): sorted(v) for k, v in sorted(writes.items())},
        "milestone_candidates": candidate_milestones,
        "issues": issues,
        "input_sha256": {str(p.relative_to(root)): sha256(p) for p in required},
        "event_sha256": event_hashes,
        "source_sha256": {str(Path(__file__).relative_to(repo)): sha256(Path(__file__)),
                          "src/Engine/Evt/EvtInstruction.cpp": sha256(repo / "src/Engine/Evt/EvtInstruction.cpp"),
                          "src/Engine/Tables/HouseTable.cpp": sha256(repo / "src/Engine/Tables/HouseTable.cpp")},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if issues:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
