#!/usr/bin/env python3
"""Audit MM6 question records and NPC topic routing from retained local data.

Reads only extracted, Git-ignored MM6 references. The detailed report stays
under that reference directory; stdout contains aggregate counts only. Handler
categories mirror the current C++ paths and do not assert original-game intent.
"""

import argparse
import csv
import hashlib
import json
import struct
from collections import Counter, defaultdict
from pathlib import Path


SOURCE_FILES = (
    "scripts/mm6_probe_dialogue_events.py",
    "src/Engine/Evt/EvtEnums.h",
    "src/Engine/Evt/EvtInstruction.cpp",
    "src/Engine/Evt/EvtInterpreter.cpp",
    "src/Engine/Evt/EvtInterpreter.h",
    "src/GUI/UI/NPCTopics.cpp",
    "src/Engine/Tables/NPCTable.cpp",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rows(path: Path):
    return csv.reader(path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t")


def active_stems(mapstats: Path) -> set[str]:
    return {Path(row[2].strip().strip('"')).stem.casefold() for row in rows(mapstats)
            if len(row) > 2 and row[0].strip().isdigit()}


def event_records(path: Path) -> tuple[dict[int, set[int]], list[dict], list[dict]]:
    data = path.read_bytes()
    steps_by_event: dict[int, set[int]] = defaultdict(set)
    questions = []
    failures = []
    offset = 0
    while offset < len(data):
        size = data[offset] + 1
        if size < 5 or offset + size > len(data):
            failures.append({"file": path.name, "offset": offset, "reason": "invalid_frame"})
            break
        event_id = struct.unpack_from("<H", data, offset + 1)[0]
        step = data[offset + 3]
        opcode = data[offset + 4]
        payload = data[offset + 5:offset + size]
        steps_by_event[event_id].add(step)
        if opcode == 26:
            question = {"file": path.name, "offset": offset, "event_id": event_id,
                        "step": step, "payload_bytes": len(payload)}
            if len(payload) == 13:
                question["string_ids"] = list(struct.unpack_from("<III", payload))
                question["target_step"] = payload[12]
            else:
                failures.append({"file": path.name, "offset": offset,
                                 "reason": "unexpected_question_payload", "bytes": len(payload)})
            questions.append(question)
        offset += size
    return steps_by_event, questions, failures


def local_string_count(path: Path) -> int:
    data = path.read_bytes()
    parts = data.split(b"\0")
    if data.endswith(b"\0"):
        parts.pop()
    return len(parts)


def current_handler(event_id: int) -> str:
    # Source: src/GUI/UI/NPCTopics.cpp, handleScriptedNPCTopicSelection.
    if event_id == 311:
        return "assert_special"
    if event_id == 139:
        return "oracle_special"
    if event_id == 399:
        return "arena_special"
    if 400 <= event_id <= 410:
        return "guild_special"
    if 200 <= event_id <= 310:
        return "teacher_special"
    return "generic_global_event"


def npc_topic_refs(npcdata: Path, topic_labels: dict[int, bool], global_ids: set[int]) -> tuple[int, list[dict], list[dict]]:
    numeric_rows = 0
    references = []
    failures = []
    for line, row in enumerate(rows(npcdata), 1):
        if not row or not row[0].strip().isdigit():
            continue
        numeric_rows += 1
        if len(row) < 13:
            failures.append({"file": npcdata.name, "line": line, "reason": "short_npc_row"})
            continue
        npc_id = int(row[0].strip())
        for slot, col in enumerate((10, 11, 12), 1):
            token = row[col].strip().strip('"')
            try:
                event_id = int(token)
            except ValueError:
                failures.append({"file": npcdata.name, "line": line,
                                 "slot": slot, "reason": "invalid_topic_id"})
                continue
            if not event_id:
                continue
            references.append({"npc_id": npc_id, "line": line, "slot": slot,
                               "event_id": event_id,
                               "global_event_present": event_id in global_ids,
                               "current_handler": current_handler(event_id),
                               "topic_label_present": topic_labels.get(event_id, False)})
    return numeric_rows, references, failures


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="retained ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="detailed JSON report inside that directory")
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

    icons = root / "icons"
    mapstats = icons / "mapstats.txt"
    npcdata = icons / "npcdata.txt"
    npctopic = icons / "npctopic.txt"
    required = (mapstats, npcdata, npctopic, icons / "global.evt")
    if not all(path.is_file() for path in required):
        parser.error("retained mapstats, npcdata, npctopic and global.evt are required")

    stems = active_stems(mapstats) | {"global"}
    events = {path.stem.casefold(): path for path in icons.glob("*.evt")
              if path.stem.casefold() in stems}
    missing_events = sorted(stems - events.keys())
    all_questions = []
    failures = []
    event_hashes = {}
    string_hashes = {}
    global_ids = set()
    for stem, path in sorted(events.items()):
        steps, questions, parse_failures = event_records(path)
        event_hashes[path.name] = sha256(path)
        failures.extend(parse_failures)
        if stem == "global":
            global_ids = set(steps)
        string_path = icons / f"{stem}.str"
        count = local_string_count(string_path) if string_path.is_file() else None
        if string_path.is_file():
            string_hashes[string_path.name] = sha256(string_path)
        for question in questions:
            if "string_ids" in question:
                question["local_string_count"] = count
                question["string_ids_in_range"] = [count is not None and value < count
                                                   for value in question["string_ids"]]
                question["target_step_exists"] = question["target_step"] in steps[question["event_id"]]
            all_questions.append(question)

    topic_labels = {}
    for row in rows(npctopic):
        if len(row) > 1 and row[0].strip().isdigit():
            topic_labels[int(row[0].strip())] = bool(row[1].strip().strip('"'))
    npc_rows, references, npc_failures = npc_topic_refs(npcdata, topic_labels, global_ids)
    failures.extend(npc_failures)
    missing_refs = [ref for ref in references if not ref["global_event_present"]]
    unclassified = [ref for ref in missing_refs if ref["current_handler"] == "generic_global_event"]
    valid_questions = [q for q in all_questions if "string_ids" in q]
    summary = {
        "active_evt_files": len(events), "missing_active_evt_files": len(missing_events),
        "question_records": len(all_questions),
        "question_maps": len({q["file"] for q in all_questions}),
        "question_payload_13_bytes": len(valid_questions),
        "question_string_references": 3 * len(valid_questions),
        "question_strings_in_range": sum(sum(q["string_ids_in_range"]) for q in valid_questions),
        "question_targets_exist": sum(q["target_step_exists"] for q in valid_questions),
        "npc_numeric_rows": npc_rows,
        "npc_nonzero_topic_references": len(references),
        "npc_refs_with_global_event": len(references) - len(missing_refs),
        "npc_refs_without_global_event": len(missing_refs),
        "missing_global_refs_by_current_handler": dict(sorted(Counter(
            ref["current_handler"] for ref in missing_refs).items())),
        "unclassified_missing_topic_refs": len(unclassified),
        "unclassified_missing_topic_ids": len({ref["event_id"] for ref in unclassified}),
        "unclassified_missing_refs_with_label": sum(ref["topic_label_present"] for ref in unclassified),
        "structural_failures": len(failures),
    }
    result = {
        "format": 1,
        "scope": "retained MM6 references only; no original archive read",
        "interpretation": "Unclassified topic references need semantic review; absent global scripts do not prove an original-game bug.",
        "summary": summary,
        "questions": all_questions,
        "npc_topic_references": references,
        "missing_active_evt_files": missing_events,
        "structural_failures": failures,
        "input_sha256": {
            "mapstats.txt": sha256(mapstats), "npcdata.txt": sha256(npcdata),
            "npctopic.txt": sha256(npctopic), "events": event_hashes,
            "local_strings": string_hashes,
        },
        "source_sha256": {name: sha256(repo / name) for name in SOURCE_FILES},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if failures or missing_events or any(not all(q["string_ids_in_range"]) or not q["target_step_exists"]
                                       for q in valid_questions):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
