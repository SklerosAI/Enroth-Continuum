#!/usr/bin/env python3
"""Export accepted F3 MM6 room descriptors to an owner-only profile manifest."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path


def digest(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def build_manifest(report_bytes: bytes, gate: dict, house_probe: dict, table_bytes: bytes) -> dict:
    report_hash = digest(report_bytes)
    if gate.get("reports_sha256", {}).get("F3_PE_RECONCILIATION_2026-11-05_v2.json") != report_hash:
        raise ValueError("corrected PE report does not match accepted F3 gate")
    if house_probe.get("format") != 2 or house_probe.get("input_sha256", {}).get("icons/2devents.txt") != digest(table_bytes):
        raise ValueError("retained house report/table identity mismatch")

    report = json.loads(report_bytes)
    if report.get("status") != "corrected_authoritative_static_analysis" or report.get("format") != 1:
        raise ValueError("expected corrected F3 PE v2 report")
    source_rows = report["house_movies"]["rows"]
    if not isinstance(source_rows, list) or len(source_rows) != 119:
        raise ValueError("expected 119 MM6 room descriptors")

    table_rows = house_probe["house_rows"]
    if len(table_rows) != 557:
        raise ValueError("expected 557 retained house rows")
    referenced = {row["room_id"] for row in table_rows.values() if row["room_id"] is not None}
    if len(referenced) != 116 or min(referenced) != 1 or max(referenced) != 118:
        raise ValueError("retained room reference set is incomplete")

    rooms = []
    names = set()
    for room_id, row in enumerate(source_rows):
        if row.get("id") != room_id:
            raise ValueError("MM6 room IDs are missing or out of order")
        file_name = row.get("file_name")
        if not isinstance(file_name, dict):
            raise ValueError("MM6 room file descriptor is malformed")
        name = file_name.get("value", "")
        if room_id == 0:
            if file_name.get("status") != "not_file_backed" or name:
                raise ValueError("MM6 room zero must have no file-backed name")
        else:
            if file_name.get("status") != "file_backed" or not isinstance(name, str) or not re.fullmatch(r"[A-Za-z0-9]{1,8}", name):
                raise ValueError("MM6 room video name is invalid")
            if name.casefold() in names:
                raise ValueError("duplicate MM6 room video name")
            names.add(name.casefold())
        for field, minimum, maximum in (("background", 0, 255), ("npc_picture", 0, 65535),
                                        ("house_type", 0, 255), ("sound_group", 0, 255)):
            number = row.get(field)
            if type(number) is not int or not minimum <= number <= maximum:
                raise ValueError(f"invalid MM6 room {field}")
        if (room_id in referenced) != bool(row.get("referenced")):
            raise ValueError("MM6 room reference flags disagree with retained house table")
        if room_id in referenced and not row.get("video_name_match"):
            raise ValueError("referenced MM6 room video name is not in accepted media index")
        rooms.append({"id": room_id, "video_name": name,
                      "background": row["background"], "npc_picture": row["npc_picture"],
                      "house_type": row["house_type"], "sound_group": row["sound_group"]})
    return {"format": 1, "source_report_sha256": report_hash,
            "source_house_table_sha256": digest(table_bytes), "rooms": rooms}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--gate", type=Path, required=True)
    parser.add_argument("--house-probe", type=Path, required=True)
    parser.add_argument("--house-table", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    rules_root = Path.home() / "Library/Application Support/Enroth Continuum/Profiles/MM6/rules"
    if args.output != rules_root / "house-rooms.json" or not rules_root.is_dir() or args.output.is_symlink():
        parser.error("output must be the private MM6 profile room manifest")
    for parent in (rules_root, rules_root.parent, rules_root.parent.parent,
                   rules_root.parent.parent.parent):
        if parent.is_symlink():
            parser.error("MM6 profile path must not use symlink parents")
    if rules_root.stat().st_mode & 0o077:
        parser.error("MM6 rules directory must be owner-only")

    manifest = build_manifest(args.report.read_bytes(), json.loads(args.gate.read_text()),
                              json.loads(args.house_probe.read_text()), args.house_table.read_bytes())
    fd = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, separators=(",", ":"))
        stream.write("\n")
    print("wrote private MM6 room descriptor manifest")


if __name__ == "__main__":
    main()
