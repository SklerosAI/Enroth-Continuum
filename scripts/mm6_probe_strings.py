#!/usr/bin/env python3
"""Audit private MM6 map strings against their active event references.

The current engine splits each map's .str file on NUL and loads an EVT by the
same map basename. This probe mirrors that indexing without copying text into
the report. Output must stay inside the ignored MM6 research directory.
"""

import argparse
import csv
import hashlib
import json
import struct
from collections import Counter, defaultdict
from pathlib import Path


STRING_OPCODES = {4, 29, 30}
OPAQUE_OPCODE = 5  # MM6 editor calls this EventMazeInfo; meaning is unverified.


def strings(path: Path) -> tuple[int, str]:
    data = path.read_bytes()
    values = data.split(b"\0")
    if data.endswith(b"\0"):
        values.pop()
    return len(values), hashlib.sha256(data).hexdigest()


def event_records(path: Path) -> dict[int, list[tuple[int, int, int]]]:
    data = path.read_bytes()
    result: dict[int, list[tuple[int, int, int]]] = defaultdict(list)
    offset = 0
    while offset < len(data):
        size = data[offset] + 1
        if size < 5 or offset + size > len(data):
            raise ValueError(f"invalid EVT record at byte {offset} of {path.name}")
        record = data[offset:offset + size]
        opcode = record[4]
        if opcode in STRING_OPCODES or opcode == OPAQUE_OPCODE:
            required = 6 if opcode in (4, 5) else 9
            if size != required:
                raise ValueError(f"unexpected opcode {opcode} size in {path.name}: {size}")
            string_id = record[5] if opcode in (4, 5) else struct.unpack_from("<I", record, 5)[0]
            result[int.from_bytes(record[1:3], "little")].append((record[3], opcode, string_id))
        elif opcode == 2:
            result[int.from_bytes(record[1:3], "little")].append((record[3], opcode, -1))
        offset += size
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="JSON report inside reference directory")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents:
        parser.error("report must remain in the private reference directory")
    icons = root / "icons"
    mapstats = icons / "mapstats.txt"
    if not mapstats.is_file():
        parser.error("mapstats.txt is missing")
    rows = csv.reader(mapstats.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t")
    map_names = sorted({row[2].strip().lower().rsplit(".", 1)[0] for row in rows
                        if len(row) > 2 and row[0].strip().isdigit()})
    report_maps = []
    missing = []
    totals: Counter[str] = Counter()
    for name in map_names:
        string_path = icons / f"{name}.str"
        event_path = icons / f"{name}.evt"
        if not string_path.is_file() or not event_path.is_file():
            missing.append(name)
            continue
        count, digest = strings(string_path)
        events = event_records(event_path)
        categories: Counter[str] = Counter()
        exceptions = []
        opaque_values = []
        for event_id, records in events.items():
            house_event = any(opcode == 2 for _, opcode, _ in records)
            for step, opcode, string_id in records:
                if opcode == OPAQUE_OPCODE:
                    opaque_values.append({"event_id": event_id, "step": step,
                                          "value": string_id,
                                          "within_local_string_range": string_id < count})
                    continue
                if opcode not in STRING_OPCODES:
                    continue
                if string_id < count:
                    category = "in_range"
                elif opcode == 4 and house_event:
                    category = "out_of_range_house_name_override"
                else:
                    category = "out_of_range_direct"
                    exceptions.append({"event_id": event_id, "step": step,
                                       "opcode": opcode, "string_id": string_id})
                categories[category] += 1
                totals[category] += 1
        report_maps.append({"name": name, "string_file_sha256": digest,
                            "string_count": count, "references": dict(categories),
                            "direct_exceptions": exceptions,
                            "opcode5_opaque_values": opaque_values})

    all_strings = sorted(path.stem.lower() for path in icons.glob("*.str"))
    unlisted = sorted(set(all_strings) - set(map_names))
    result = {"format": 1, "maps": report_maps,
              "summary": {"listed_maps": len(map_names), "matched_maps": len(report_maps),
                          "missing_pairs": missing, "all_string_files": len(all_strings),
                          "unlisted_string_files": unlisted,
                          "reference_categories": dict(totals),
                          "direct_exceptions": sum(len(m["direct_exceptions"]) for m in report_maps),
                          "opcode5_records": sum(len(m["opcode5_opaque_values"]) for m in report_maps),
                          "opcode5_outside_string_range": sum(
                              not v["within_local_string_range"]
                              for m in report_maps for v in m["opcode5_opaque_values"])}}
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Checked {len(report_maps)} map string/EVT pairs; "
          f"{result['summary']['direct_exceptions']} direct out-of-range references")
    if missing or len(report_maps) != 67:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
