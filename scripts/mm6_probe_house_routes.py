#!/usr/bin/env python3
"""Join retained MM6 house rows to active EnterHouse events and current UI routing.

Detailed names and event locations stay in an ignored local JSON report.
This F2 source/data probe does not establish the original house behavior.
"""

import argparse
import csv
import hashlib
import json
import re
import struct
from collections import Counter, defaultdict
from pathlib import Path


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rows(path: Path) -> list[list[str]]:
    return list(csv.reader(path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t"))


def known_house_labels(path: Path, map_name: str) -> set[str]:
    source = path.read_text(encoding="utf-8")
    section = source.split(f"{map_name} = {{", 1)[1].split("};", 1)[0]
    return {name.casefold() for name in re.findall(r'\{\s*"([^"]+)"\s*,', section)}


def mm6_type_is_classified(label: str, known: set[str]) -> bool:
    return label.casefold() in known or bool(re.fullmatch(r"(?:house|hosue) [pmr][0-9]+", label, re.I))


def event_house_refs(icons: Path, active_stems: set[str]) -> tuple[list[dict], list[dict], dict[str, str]]:
    refs = []
    failures = []
    fingerprints = {}
    for path in sorted(icons.glob("*.evt")):
        if path.stem.casefold() not in active_stems | {"global"}:
            continue
        raw = path.read_bytes()
        fingerprints[path.name] = hashlib.sha256(raw).hexdigest()
        offset = 0
        while offset < len(raw):
            size = raw[offset] + 1
            if size < 5 or offset + size > len(raw):
                failures.append({"file": path.name, "offset": offset, "reason": "invalid frame"})
                break
            opcode = raw[offset + 4]
            if opcode == 2:
                payload = raw[offset + 5:offset + size]
                if len(payload) != 4:
                    failures.append({"file": path.name, "offset": offset, "reason": "unexpected EnterHouse payload"})
                else:
                    refs.append({"file": path.name,
                                 "event_id": struct.unpack_from("<H", raw, offset + 1)[0],
                                 "offset": offset,
                                 "house_id": struct.unpack_from("<I", payload)[0]})
            offset += size
    return refs, failures, fingerprints


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="detailed JSON report inside that directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    reference = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if reference == allowed or not reference.is_relative_to(allowed):
        parser.error("reference must be an ignored MM6 research directory")
    if not report.is_relative_to(reference) or report.suffix.lower() != ".json" or not report.parent.is_dir():
        parser.error("JSON report must stay inside the ignored reference directory")

    icons = reference / "icons"
    inputs = [icons / "mapstats.txt", icons / "2devents.txt"]
    sources = [repo / "src/Engine/Tables/HouseTable.cpp", repo / "src/GUI/UI/UIHouses.cpp",
               repo / "src/Engine/Evt/EvtInstruction.cpp", repo / "src/Engine/Evt/EvtInterpreter.cpp",
               repo / "scripts/mm6_probe_house_routes.py"]
    missing = [str(path) for path in inputs + sources if not path.is_file()]
    if missing:
        parser.error("missing input/source: " + ", ".join(missing))

    map_rows = [row for row in rows(inputs[0]) if len(row) > 2 and row[0].strip().isdigit()]
    active_stems = {Path(row[2].strip().strip('"')).stem.casefold() for row in map_rows}
    house_rows = {}
    for row in rows(inputs[1]):
        if not row or not row[0].strip().isdigit():
            continue
        house_id = int(row[0])
        if house_id in house_rows:
            raise ValueError("duplicate house ID")
        padded = row + [""] * max(0, 24 - len(row))
        room_token = padded[4].strip()
        house_rows[house_id] = {"type": padded[2].strip(), "room_id": int(room_token) if room_token.isdigit() else None,
                                "room_token": room_token, "raw_column_count": len(row)}

    legacy_known = known_house_labels(sources[0], "houseTypeMap")
    mm6_known = legacy_known | known_house_labels(sources[0], "mm6Types")
    refs, failures, event_hashes = event_house_refs(icons, active_stems)
    active_ids = {ref["house_id"] for ref in refs}
    special_or_missing = sorted(active_ids - house_rows.keys())
    active_rows = {house_id: house_rows[house_id] for house_id in active_ids & house_rows.keys()}
    legacy_fallback_ids = {house_id for house_id, row in active_rows.items()
                           if row["type"] and row["type"].casefold() not in legacy_known}
    unclassified_ids = {house_id for house_id, row in active_rows.items()
                        if row["type"] and not mm6_type_is_classified(row["type"], mm6_known)}
    blank_type_ids = {house_id for house_id, row in active_rows.items() if not row["type"]}
    room_groups = defaultdict(set)
    for house_id, row in active_rows.items():
        if row["room_id"] is not None:
            room_groups[row["room_id"]].add(row["type"])
    summary = {
        "map_rows": len(map_rows),
        "active_evt_files": len(event_hashes),
        "house_table_rows": len(house_rows),
        "house_type_labels": len({row["type"] for row in house_rows.values()}),
        "house_type_labels_unmapped_in_legacy_parser": len({row["type"] for row in house_rows.values()
                                                              if row["type"] and row["type"].casefold() not in legacy_known}),
        "house_table_rows_falling_to_legacy_mercenary_type": sum(
            bool(row["type"]) and row["type"].casefold() not in legacy_known for row in house_rows.values()),
        "house_table_rows_unclassified_in_mm6_parser": sum(
            bool(row["type"]) and not mm6_type_is_classified(row["type"], mm6_known)
            for row in house_rows.values()),
        "enter_house_instructions": len(refs),
        "distinct_enter_house_ids": len(active_ids),
        "distinct_enter_house_ids_with_table_row": len(active_rows),
        "distinct_enter_house_ids_outside_table": len(special_or_missing),
        "active_house_ids_falling_to_legacy_mercenary_type": len(legacy_fallback_ids),
        "active_house_ids_unclassified_in_mm6_parser": len(unclassified_ids),
        "active_house_ids_with_blank_type": len(blank_type_ids),
        "active_house_ids_with_room_id": sum(row["room_id"] is not None for row in active_rows.values()),
        "active_room_ids_shared_across_types": sum(len(types) > 1 for types in room_groups.values()),
        "event_parse_failures": len(failures),
    }
    result = {
        "format": 2,
        "scope": "retained MM6 table/event copy and current source; no original archives or runtime opened",
        "warning": "Type classification does not prove original house UI, services or movie mapping.",
        "summary": summary,
        "known_legacy_type_labels": sorted(legacy_known),
        "known_mm6_explicit_type_labels": sorted(mm6_known),
        "house_rows": house_rows,
        "enter_house_refs": refs,
        "special_or_missing_house_ids": special_or_missing,
        "legacy_fallback_house_ids": sorted(legacy_fallback_ids),
        "active_unclassified_house_ids": sorted(unclassified_ids),
        "active_blank_type_house_ids": sorted(blank_type_ids),
        "active_room_type_groups": {str(room_id): sorted(types) for room_id, types in sorted(room_groups.items())},
        "event_parse_failures": failures,
        "input_sha256": {str(path.relative_to(reference)): digest(path) for path in inputs},
        "event_sha256": event_hashes,
        "source_sha256": {str(path.relative_to(repo)): digest(path) for path in sources},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    report.chmod(0o600)
    print(json.dumps(summary, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
