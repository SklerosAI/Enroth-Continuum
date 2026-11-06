#!/usr/bin/env python3
"""Join retained MM6 EVT parser failures to event/variable coverage.

Reads only ignored F2 reports. Exact file/event evidence stays in an ignored
JSON output; stdout is aggregate. Never opens the original installation.
"""

import argparse
import hashlib
import json
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_json(path: Path) -> dict:
    return json.loads(path.read_text())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    output = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must stay in ignored local-game-data/mm6")
    if output.suffix.lower() != ".json" or not output.is_relative_to(root) or not output.parent.is_dir():
        parser.error("report must be JSON inside the ignored reference directory")

    inputs = {
        "event-parser-probe.json": root / "event-parser-probe.json",
        "opcode-semantics-probe.json": root / "opcode-semantics-probe.json",
        "event-variable-probe.json": root / "event-variable-probe.json",
    }
    source = {
        "src/Engine/Evt/EvtInstruction.cpp": repo / "src/Engine/Evt/EvtInstruction.cpp",
        "src/Engine/Evt/EvtProgram.cpp": repo / "src/Engine/Evt/EvtProgram.cpp",
        "src/Engine/Evt/EvtInterpreter.cpp": repo / "src/Engine/Evt/EvtInterpreter.cpp",
        "src/Engine/Evt/Processor.cpp": repo / "src/Engine/Evt/Processor.cpp",
        "src/Engine/Engine.cpp": repo / "src/Engine/Engine.cpp",
        "src/Engine/Objects/CombinedSkillValue.cpp": repo / "src/Engine/Objects/CombinedSkillValue.cpp",
        "src/Engine/Objects/CharacterEnums.h": repo / "src/Engine/Objects/CharacterEnums.h",
    }
    parser_data = read_json(inputs["event-parser-probe.json"])
    opcode_data = read_json(inputs["opcode-semantics-probe.json"])
    variable_data = read_json(inputs["event-variable-probe.json"])
    if parser_data["cpp_parser_sha256"] != sha256(source["src/Engine/Evt/EvtInstruction.cpp"]):
        parser.error("retained parser report is stale against current source")
    if (parser_data["summary"]["active_files"], parser_data["summary"]["active_records"],
            parser_data["summary"]["deterministic_parser_failures"]) != (68, 14477, 318):
        parser.error("unexpected parser report coverage")
    if (opcode_data["summary"]["active_opcodes"],
            variable_data["summary"]["distinct_variable_ids"],
            variable_data["summary"]["variable_records"]) != (43, 127, 4285):
        parser.error("unexpected opcode or variable report coverage")

    failures = [record for records in parser_data["failures"].values() for record in records]
    blocked = {record["file"] for record in failures}
    records = [record for entry in opcode_data["opcodes"].values() for record in entry["records"]]
    variables = variable_data["records"]
    if len(records) != 14477 or len(failures) != 318:
        parser.error("report body does not match summary")
    if "global.evt" in blocked or len(blocked) != 26:
        parser.error("unexpected affected-file set")

    skill_records = opcode_data["opcodes"]["43"]["records"]
    if len(skill_records) != 1:
        parser.error("expected one retained CheckSkill record")
    skill_record = skill_records[0]
    skill_bytes = bytes.fromhex(skill_record["payload_hex"])
    if len(skill_bytes) != 7:
        parser.error("unexpected CheckSkill payload width")
    encoded_mastery = skill_bytes[1]
    minimum_level = int.from_bytes(skill_bytes[2:6], "little")
    if encoded_mastery != 0 or minimum_level != 8:
        parser.error("CheckSkill sentinel changed; revisit source interpretation")

    summary = {
        "active_event_files": 68,
        "map_event_files_rejected_by_current_parser": len(blocked),
        "active_records_in_rejected_files": sum(record["file"] in blocked for record in records),
        "variable_operands_in_rejected_files": sum(record["file"] in blocked for record in variables),
        "distinct_variable_ids_in_rejected_files": len({record["variable_id"] for record in variables if record["file"] in blocked}),
        "all_active_opcodes": 43,
        "all_active_variable_ids": 127,
        "checkskill_record_count": len(skill_records),
        "checkskill_encoded_mastery": encoded_mastery,
        "checkskill_required_level": minimum_level,
        "checkskill_in_parser_rejected_file": skill_record["file"] in blocked,
    }
    private = {
        "format": 1,
        "scope": "ignored retained F2 reports plus current source; no original install or MM6 launch",
        "summary": summary,
        "rejected_event_files": sorted(blocked),
        "rejected_files_by_reason": {reason: sorted({record["file"] for record in entries})
                                     for reason, entries in parser_data["failures"].items()},
        "checkskill_record": skill_record,
        "input_sha256": {name: sha256(path) for name, path in inputs.items()},
        "source_sha256": {name: sha256(path) for name, path in source.items()},
    }
    output.write_text(json.dumps(private, indent=2) + "\n")
    print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    main()
