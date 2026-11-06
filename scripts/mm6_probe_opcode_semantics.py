#!/usr/bin/env python3
"""Inventory active retained MM6 EVT payloads and inherited opcode coverage.

Reads only extracted, ignored MM6 references. Exact event IDs and payload bytes
are written only to an ignored local report; stdout is aggregate. This is a
structural F2 probe, not a semantic emulator or an F3 original-archive read.
"""

import argparse
import hashlib
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = {
    "enum": ROOT / "src/Engine/Evt/EvtEnums.h",
    "parser": ROOT / "src/Engine/Evt/EvtInstruction.cpp",
    "executor": ROOT / "src/Engine/Evt/EvtInterpreter.cpp",
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_coverage() -> dict[int, dict]:
    enum = SOURCE["enum"].read_text(encoding="utf-8")
    parser = SOURCE["parser"].read_text(encoding="utf-8")
    executor = SOURCE["executor"].read_text(encoding="utf-8")
    enum_section = enum.split("enum class EvtOpcode", 1)[1].split("};", 1)[0]
    parse_section = parser.split("EvtInstruction EvtInstruction::parse(", 1)[1]
    execute_section = executor.split("switch (ir.opcode)", 1)[1].split("switch (flow.type)", 1)[0]
    names = {int(value): name for name, value in
             re.findall(r"\b(EVENT_\w+)\s*=\s*(\d+)", enum_section)}
    result = {}
    for opcode, name in names.items():
        result[opcode] = {"inherited_enum": name,
                          "parser_case": bool(re.search(r"\bcase\s+" + name + r"\s*:", parse_section)),
                          "executor_case": bool(re.search(r"\bcase\s+" + name + r"\s*:", execute_section))}
    return result


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("reference", type=Path, help="ignored MM6 research directory")
    ap.add_argument("report", type=Path, help="JSON report inside reference")
    args = ap.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents or ROOT / "local-game-data/mm6" not in root.parents:
        ap.error("reference and report must remain under ignored local-game-data/mm6")
    inventory = root / "event-probe.json"
    if not inventory.is_file():
        ap.error("run the retained event inventory first")
    prior = json.loads(inventory.read_text(encoding="utf-8"))
    active = [f for f in prior["files"] if f["role"] in ("listed_map", "global")]
    if len(active) != 68 or any("error" in f for f in active):
        ap.error("expected 68 well-framed listed-map/global EVT files")

    coverage = source_coverage()
    occurrences = defaultdict(list)
    for entry in active:
        path = root / "icons" / entry["name"]
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() != entry["sha256"]:
            ap.error(f"retained EVT changed after inventory: {entry['name']}")
        for record in entry["records"]:
            offset, size = record["offset"], record["size"]
            frame = data[offset:offset + size]
            if (len(frame) != size or frame[0] + 1 != size or
                    int.from_bytes(frame[1:3], "little") != record["event_id"] or
                    frame[3] != record["step"] or frame[4] != record["opcode"]):
                ap.error(f"event inventory disagreement: {entry['name']} at {offset}")
            occurrences[record["opcode"]].append({
                "file": entry["name"], "role": entry["role"], "offset": offset,
                "event_id": record["event_id"], "step": record["step"],
                "payload_length": size - 5, "payload_hex": frame[5:].hex(),
            })
    if len(occurrences) != 43 or sum(map(len, occurrences.values())) != 14477:
        ap.error("active opcode/record count changed; re-run F2 inventory")
    rows = {}
    for opcode, records in sorted(occurrences.items()):
        rows[str(opcode)] = {
            "count": len(records),
            "payload_length_counts": dict(sorted(Counter(r["payload_length"] for r in records).items())),
            "active_files": len({r["file"] for r in records}),
            "current_cpp": coverage.get(opcode, {"inherited_enum": None, "parser_case": False,
                                                  "executor_case": False}),
            "records": records,
        }
    result = {
        "format": 1,
        "scope": "retained listed-map/global EVT only; no original archive read or game execution",
        "summary": {"active_files": len(active), "active_records": 14477,
                    "active_opcodes": len(rows),
                    "missing_parser_cases": [int(k) for k, v in rows.items() if not v["current_cpp"]["parser_case"]],
                    "missing_executor_cases": [int(k) for k, v in rows.items() if not v["current_cpp"]["executor_case"]]},
        "opcodes": rows,
        "input_sha256": {"event_probe": sha256(inventory),
                          **{entry["name"]: entry["sha256"] for entry in active}},
        "source_sha256": {name: sha256(path) for name, path in SOURCE.items()},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result["summary"], indent=2))
    print("Active opcode payload widths (bytes after 5-byte frame header):")
    for opcode, row in rows.items():
        widths = ",".join(str(width) for width in row["payload_length_counts"])
        print(f"{int(opcode):2d}: {row['count']:4d} records; widths {widths}")


if __name__ == "__main__":
    main()
