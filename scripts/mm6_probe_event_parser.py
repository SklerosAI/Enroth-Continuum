#!/usr/bin/env python3
"""Find deterministic MM6 EVT failures in the current inherited parser.

This audits already extracted private event files. The frame minima and fixed
operand widths below mirror EvtInstruction::parse in this checkout; re-review
them whenever that C++ parser changes. It does not execute game events.
"""

import argparse
import csv
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path


# Total record bytes including the one-byte length and four-byte EVT header.
# Source: src/Engine/Evt/EvtInstruction.cpp, EvtInstruction::parse.
CURRENT_MINIMUM = {
    1: 6, 2: 6, 3: 17, 4: 6, 5: 6, 6: 32, 7: 6, 8: 7, 9: 11,
    10: 7, 11: 10, 12: 8, 13: 11, 14: 10, 15: 7, 16: 10, 17: 10,
    18: 10, 19: 28, 21: 32, 22: 9, 23: 14, 24: 14, 25: 11,
    26: 9, 29: 9, 30: 9, 31: 15, 32: 10, 33: 6, 34: 27,
    35: 6, 36: 6, 37: 6, 38: 15, 39: 14, 40: 10, 41: 11,
    42: 9, 43: 12,
}
UNIMPLEMENTED_OPCODES = {20, 27, 28}
# The current C++ cases read this many bytes and then require end-of-record.
FIXED_CONSUMED_PAYLOAD = {24: 9, 26: 4}


def listed_map_stems(path: Path) -> set[str]:
    rows = csv.reader(path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t")
    return {Path(row[2].strip().strip('"')).stem.casefold() for row in rows
            if len(row) > 2 and row[0].strip().isdigit()}


def classify(opcode: int, size: int, payload: bytes) -> str | None:
    if opcode in UNIMPLEMENTED_OPCODES:
        return "missing_case"
    minimum = CURRENT_MINIMUM.get(opcode)
    if minimum is None:
        return "unreviewed_opcode"
    if size < minimum:
        return "minimum_size_rejection"
    if opcode in FIXED_CONSUMED_PAYLOAD and len(payload) != FIXED_CONSUMED_PAYLOAD[opcode]:
        return "leftover_bytes"
    if opcode == 12:
        # Current EVENT_ShowMovie consumes two bytes, then a NUL-terminated
        # string. In MM6's active records those two bytes begin a 32-bit ID.
        terminator = payload.find(b"\0", 2)
        if terminator < 0 or terminator != len(payload) - 1:
            return "movie_parser_leftover"
    return None


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="output JSON inside that directory")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents:
        parser.error("report must remain in the private reference directory")
    icons = root / "icons"
    mapstats = icons / "mapstats.txt"
    if not mapstats.is_file():
        parser.error("retained mapstats.txt is required")
    active_stems = listed_map_stems(mapstats) | {"global"}

    counts = Counter()
    failures = defaultdict(list)
    malformed = []
    files = 0
    for path in sorted(icons.glob("*.evt")):
        if path.stem.casefold() not in active_stems:
            continue
        files += 1
        data = path.read_bytes()
        offset = 0
        while offset < len(data):
            size = data[offset] + 1
            if size < 5 or offset + size > len(data):
                malformed.append({"file": path.name, "offset": offset})
                break
            opcode = data[offset + 4]
            payload = data[offset + 5:offset + size]
            counts[opcode] += 1
            reason = classify(opcode, size, payload)
            if reason:
                failures[reason].append({"file": path.name, "offset": offset,
                                         "opcode": opcode, "bytes": size})
            offset += size

    summary = {"active_files": files, "active_records": sum(counts.values()),
               "active_opcodes": len(counts),
               "deterministic_parser_failures": sum(map(len, failures.values())),
               "failure_counts": {key: len(value) for key, value in sorted(failures.items())},
               "failed_opcodes": dict(sorted(Counter(record["opcode"] for values in failures.values()
                                                    for record in values).items())),
               "malformed_files": malformed}
    result = {"format": 1, "summary": summary,
              "event_opcode_counts": dict(sorted(counts.items())),
              "failures": dict(sorted(failures.items())),
              "cpp_parser_sha256": hashlib.sha256((Path(__file__).resolve().parents[1] /
                  "src/Engine/Evt/EvtInstruction.cpp").read_bytes()).hexdigest(),
              "mapstats_sha256": hashlib.sha256(mapstats.read_bytes()).hexdigest()}
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if malformed or files != len(active_stems) or set(counts) - (set(CURRENT_MINIMUM) | UNIMPLEMENTED_OPCODES):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
