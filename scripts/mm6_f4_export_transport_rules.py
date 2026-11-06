#!/usr/bin/env python3
"""Export F3-matched MM6 travel constants to a private profile manifest.

The generated file contains proprietary derived game data. Keep it under the
ignored MM6 user profile; do not commit or upload it.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path


KINDS = {
    None: "blank",
    "schedule_candidate": "scheduled",
    "schedule_candidate_with_unknown_day": "weekday_uncertain",
    "opaque_special": "special_unresolved",
}


def export(report_path: Path) -> dict:
    report_path = report_path.resolve(strict=True)
    root = report_path.parent
    if root.parent.name != "mm6" or root.parent.parent.name != "local-game-data":
        raise ValueError("source report must be in ignored local-game-data/mm6")
    gate = json.loads((root / "F3_GATE_REPORT_HASHES_2026-11-06.json").read_text())
    digest = hashlib.sha256(report_path.read_bytes()).hexdigest()
    if gate["reports_sha256"].get(report_path.name) != digest:
        raise ValueError("F3 travel report differs from signed gate")
    report = json.loads(report_path.read_text())
    if report["status"] != "corrected_authoritative_static_analysis":
        raise ValueError("F3 travel report is not the corrected static analysis")
    travel = report["transport"]["travel_rows"]
    joins = report["transport"]["house_slot_joins"]
    if len(travel) != 36 or len(joins) != 63:
        raise ValueError("F3 travel row count changed")

    routes = []
    for index, row in enumerate(travel):
        days = row["days_available"]
        position = row["position"]
        if (row["id"] != index or not 1 <= row["map_index"] <= 256 or
                not 1 <= row["days_count"] <= 30 or len(days) != 7 or
                any(day not in (0, 1) for day in days) or len(position) != 3 or
                any(not isinstance(axis, int) or abs(axis) > 100000 for axis in position) or
                not 0 <= row["direction"] < 2048 or not 0 <= row["quest_bit"] < 512 or
                not row["destination_in_retained_map_probe"]):
            raise ValueError("invalid F3 travel descriptor")
        routes.append({"map_index": row["map_index"], "days": days,
                       "duration": row["days_count"], "position": position,
                       "yaw": row["direction"], "quest_bit": row["quest_bit"]})

    houses = []
    for house_id in range(48, 69):
        slots = [row for row in joins if row["house_id"] == house_id]
        if len(slots) != 3 or {row["slot"] for row in slots} != {"A", "B", "C"}:
            raise ValueError("invalid F3 house slot layout")
        output = []
        for slot in sorted(slots, key=lambda row: row["slot"]):
            kind = KINDS.get(slot["cell_kind"])
            if kind is None or slot["cell_present"] != (kind != "blank") or not slot["valid_route_index"]:
                raise ValueError("invalid F3 travel slot")
            index = slot["route_index"]
            if not 0 <= index < len(routes):
                raise ValueError("travel slot references missing descriptor")
            output.append({"kind": kind, "route": index})
        houses.append({"id": house_id, "slots": output})
    if sum(slot["kind"] != "blank" for house in houses for slot in house["slots"]) != 37:
        raise ValueError("F3 populated travel slot count changed")
    return {"format": 1, "source_report_sha256": digest,
            "routes": routes, "houses": houses}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    result = export(args.report)
    output = args.output.expanduser()
    if output.name != "transport.json" or output.parent.name != "rules" or output.parent.parent.name != "MM6":
        parser.error("output must be MM6/rules/transport.json in the private profile")
    output.parent.mkdir(parents=True, exist_ok=True)
    data = (json.dumps(result, separators=(",", ":")) + "\n").encode()
    descriptor = os.open(output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
    except Exception:
        output.unlink(missing_ok=True)
        raise
    print("Wrote private MM6 travel manifest: 36 descriptors, 21 houses, 37 populated cells")


if __name__ == "__main__":
    main()
