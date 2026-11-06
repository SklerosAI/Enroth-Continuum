#!/usr/bin/env python3
"""Record an owner-local MM6 quest-bit EVT handoff from frozen F3 inputs."""

import csv
import hashlib
import json
import os
import tempfile
from collections import Counter
from pathlib import Path


OPCODES = {14: "Compare", 16: "Add", 17: "Subtract", 18: "Set"}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    reference = repo / "local-game-data/mm6/research-2026-11-05"
    event_path = reference / "event-variable-probe.json"
    quest_path = reference / "icons/quests.txt"
    frozen_path = reference / "F2_FREEZE_MANIFEST.json"
    destination = repo / "local-game-data/private-wiki/mm6/F4_QUEST_BIT_VARIABLE_2026-11-06.json"

    frozen = json.loads(frozen_path.read_text(encoding="utf-8"))["ignored_derived_sha256"]
    for path in (event_path, quest_path):
        key = str(path.relative_to(repo))
        if frozen.get(key) != sha256(path):
            raise ValueError(f"F3-retained input fingerprint changed: {path.name}")

    event_report = json.loads(event_path.read_text(encoding="utf-8"))
    rows = {}
    with quest_path.open(encoding="cp1252", newline="") as stream:
        for cells in csv.reader(stream, delimiter="\t"):
            if not cells or not cells[0].strip().isdigit():
                continue
            quest_id = int(cells[0].strip())
            if quest_id in rows:
                raise ValueError("duplicate retained quest ID")
            rows[quest_id] = len(cells) > 1 and cells[1].strip() not in ("", "0")
    if set(rows) != set(range(1, 513)):
        raise ValueError("retained quest table is not the expected 512-row set")

    active = []
    for record in event_report["records"]:
        if record["variable_id"] != 16:
            continue
        opcode = record["opcode"]
        value = record["value"]
        if opcode not in OPCODES or value not in rows:
            raise ValueError("unknown active quest-bit operation or operand")
        active.append({
            "file": record["file"],
            "event_id": record["event_id"],
            "offset": record["offset"],
            "operation": OPCODES[opcode],
            "quest_bit": value,
            "has_visible_text": rows[value],
        })
    if len(active) != 611:
        raise ValueError("retained quest-bit use count changed")

    uses_by_operation = Counter(row["operation"] for row in active)
    if uses_by_operation != {"Compare": 276, "Add": 59, "Subtract": 92, "Set": 184}:
        raise ValueError("retained quest-bit operation inventory changed")
    used_ids = {row["quest_bit"] for row in active}
    summary = {
        "active_uses": len(active),
        "distinct_active_bits": len(used_ids),
        "minimum_active_bit": min(used_ids),
        "maximum_active_bit": max(used_ids),
        "quest_table_rows": len(rows),
        "text_rows": sum(rows.values()),
        "active_bits_with_text": sum(rows[bit] for bit in used_ids),
        "active_bits_without_text": sum(not rows[bit] for bit in used_ids),
        "uses_by_operation": dict(sorted(uses_by_operation.items())),
        "text_uses_by_operation": {
            name: sum(row["has_visible_text"] for row in active if row["operation"] == name)
            for name in OPCODES.values()
        },
    }
    report = {
        "format": 1,
        "scope": "F3-matched retained quest table and EVT probe; no original installation or game opened",
        "summary": summary,
        "retained_sha256": {str(path.relative_to(reference)): sha256(path)
                            for path in (event_path, quest_path, frozen_path)},
        "source_sha256": {str(path.relative_to(repo)): sha256(path)
                          for path in (Path(__file__),
                                       repo / "src/Engine/Evt/EvtVariables.cpp",
                                       repo / "src/Engine/Evt/EvtInterpreter.cpp",
                                       repo / "src/Engine/Tables/QuestTable.cpp",
                                       repo / "src/Engine/MM6/MM6DurableSaveRuntime.cpp")},
        "records": active,
    }
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=destination.parent,
                                     prefix=".quest-bit-", delete=False) as stream:
        temp = Path(stream.name)
        os.fchmod(stream.fileno(), 0o600)
        json.dump(report, stream, indent=2)
        stream.write("\n")
    os.replace(temp, destination)
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
