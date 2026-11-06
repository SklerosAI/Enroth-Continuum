#!/usr/bin/env python3
"""Inventory MM6 EVT variable IDs and compare them with inherited enum names.

The MM6 ID groups follow the MM8LevelEditor MM6 event format source:
https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java
The report stays beside the already extracted private MM6 files, outside Git.
"""

import argparse
import csv
import hashlib
import json
import re
import struct
from collections import Counter, defaultdict
from pathlib import Path


MM6_RESISTANCE = (
    "fire", "electricity", "cold", "poison", "magic",
    "fire temporary", "electricity temporary", "cold temporary",
    "poison temporary", "magic temporary",
)
MM6_SKILL = (
    "staff", "sword", "dagger", "axe", "spear", "bow", "mace", "blaster",
    "shield", "leather", "chain", "plate", "fire magic", "air magic",
    "water magic", "earth magic", "spirit magic", "mind magic",
    "body magic", "light magic", "dark magic", "identify item", "merchant",
    "repair item", "body building", "meditation", "perception",
    "diplomacy", "thievery", "lock pick", "learning",
)
MM6_CONDITION = (
    "cursed", "weak", "asleep", "afraid", "drunk", "insane", "poison 1",
    "disease 1", "poison 2", "disease 2", "poison 3", "disease 3",
    "paralyzed", "unconscious", "dead", "stone", "eradicated", "good",
)
MM6_SPECIAL = {
    205: "event autonote", 206: "might max", 207: "intellect max",
    208: "personality max", 209: "endurance max", 210: "speed max",
    211: "accuracy max", 212: "luck max", 213: "character bit",
    214: "NPC in party", 215: "reputation", 222: "party flying",
    223: "NPC profession in party", 224: "circus total",
    225: "skill points", 226: "month of year",
}


def mm6_meaning(value: int) -> str:
    if value < 46:
        return "low shared ID; semantic review still required"
    if value <= 55:
        return "resistance: " + MM6_RESISTANCE[value - 46]
    if value <= 86:
        return "skill: " + MM6_SKILL[value - 56]
    if value <= 104:
        return "condition: " + MM6_CONDITION[value - 87]
    if value <= 204:
        return f"map persistent byte {value - 104}"
    if value in MM6_SPECIAL:
        return MM6_SPECIAL[value]
    if 216 <= value <= 221:
        return f"days past {value - 215}"
    if 227 <= value <= 236:
        return f"timer {value - 226}"
    return "unreviewed MM6 ID"


def inherited_names(path: Path) -> dict[int, str]:
    source = path.read_text(encoding="utf-8")
    names = {}
    for name, raw in re.findall(r"\b(VAR_\w+)\s*=\s*(0x[0-9A-Fa-f]+|[0-9]+)\s*[,}]", source):
        names[int(raw, 0)] = name
    for index in range(75):
        names.setdefault(123 + index, f"VAR_MapPersistentVariable_{index}")
    for index in range(25):
        names.setdefault(198 + index, f"VAR_MapPersistentDecorVariable_{index}")
    return names


def listed_stems(path: Path) -> set[str]:
    rows = csv.reader(path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t")
    return {Path(row[2].strip().strip('"')).stem.casefold() for row in rows
            if len(row) > 2 and row[0].strip().isdigit()}


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
    enum_file = Path(__file__).resolve().parents[1] / "src/Engine/Evt/EvtEnums.h"
    active = listed_stems(mapstats) | {"global"}
    names = inherited_names(enum_file)

    counts = Counter()
    by_opcode = defaultdict(Counter)
    rows = []
    malformed = []
    files = 0
    for path in sorted(icons.glob("*.evt")):
        if path.stem.casefold() not in active:
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
            if opcode in (14, 16, 17, 18):
                payload = data[offset + 5:offset + size]
                if len(payload) != (6 if opcode == 14 else 5):
                    malformed.append({"file": path.name, "offset": offset,
                                      "opcode": opcode, "payload_bytes": len(payload)})
                else:
                    variable_id = payload[0]
                    value = struct.unpack_from("<I", payload, 1)[0]
                    counts[variable_id] += 1
                    by_opcode[opcode][variable_id] += 1
                    rows.append({"file": path.name, "offset": offset,
                                 "event_id": struct.unpack_from("<H", data, offset + 1)[0],
                                 "opcode": opcode, "variable_id": variable_id,
                                 "value": value})
            offset += size

    ranges = {
        "low_0_45": (0, 45), "resistance_46_55": (46, 55),
        "skill_56_86": (56, 86), "condition_87_104": (87, 104),
        "map_byte_105_204": (105, 204), "party_state_205_226": (205, 226),
        "timer_227_236": (227, 236), "late_237_255": (237, 255),
    }
    summaries = {label: {"records": sum(count for value, count in counts.items() if lo <= value <= hi),
                         "distinct_ids": sum(lo <= value <= hi for value in counts)}
                 for label, (lo, hi) in ranges.items()}
    summary = {"active_files": files, "variable_records": len(rows),
               "distinct_variable_ids": len(counts), "range_counts": summaries,
               "framing_failures": malformed}
    catalog = [{"id": value, "records": count, "mm6_meaning": mm6_meaning(value),
                "inherited_name": names.get(value, "NO_ENUM_NAME"),
                "by_opcode": {opcode: op_counts[value] for opcode, op_counts in by_opcode.items()
                              if op_counts[value]}}
               for value, count in sorted(counts.items())]
    result = {"format": 1, "summary": summary, "catalog": catalog,
              "records": rows,
              "input_sha256": {"mapstats": hashlib.sha256(mapstats.read_bytes()).hexdigest(),
                               "inherited_enum": hashlib.sha256(enum_file.read_bytes()).hexdigest()}}
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if malformed or files != len(active):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
