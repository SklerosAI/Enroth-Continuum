#!/usr/bin/env python3
"""Frame and opcode inventory for already extracted private MM6 EVT files."""

import argparse
import csv
import hashlib
import json
from collections import Counter
from pathlib import Path


def probe(path: Path) -> dict:
    data = path.read_bytes()
    result = {"name": path.name, "bytes": len(data),
              "sha256": hashlib.sha256(data).hexdigest(), "records": []}
    offset = 0
    while offset < len(data):
        size = data[offset] + 1
        if size < 5 or offset + size > len(data):
            result["error"] = f"invalid record size {size} at offset {offset}"
            break
        entry = data[offset:offset + size]
        result["records"].append({"offset": offset, "size": size,
                                  "event_id": int.from_bytes(entry[1:3], "little"),
                                  "step": entry[3], "opcode": entry[4]})
        offset += size
    result["bytes_consumed"] = offset
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="ignored output JSON inside reference")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents:
        parser.error("report must remain in the private reference directory")
    files = sorted((root / "icons").glob("*.evt"))
    if not files:
        parser.error("no extracted EVT files found")
    mapstats = root / "icons" / "mapstats.txt"
    if not mapstats.is_file():
        parser.error("mapstats.txt is needed to distinguish listed map scripts")
    rows = csv.reader(mapstats.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t")
    listed_maps = {row[2].strip().lower().rsplit(".", 1)[0] for row in rows
                   if len(row) > 2 and row[0].strip().isdigit()}
    results = [probe(path) for path in files]
    for file in results:
        stem = file["name"].lower().rsplit(".", 1)[0]
        file["role"] = "listed_map" if stem in listed_maps else ("global" if stem == "global" else "unlisted")
    active = [file for file in results if file["role"] != "unlisted"]
    counts = Counter(record["opcode"] for file in active for record in file["records"])
    payload_lengths = {
        opcode: sorted({record["size"] - 5 for file in active for record in file["records"]
                        if record["opcode"] == opcode})
        for opcode in counts
    }
    games = root / "Games"
    base_map_names = {path.stem.lower() for suffix in ("*.odm", "*.blv")
                      for path in games.glob(suffix)}
    unlisted_with_base_map = [file["name"] for file in results
                              if file["role"] == "unlisted" and
                              Path(file["name"]).stem.lower() in base_map_names]
    summary = {"files": len(results), "records": sum(len(file["records"]) for file in results),
               "listed_map_files": sum(file["role"] == "listed_map" for file in results),
               "global_files": sum(file["role"] == "global" for file in results),
               "unlisted_files": [file["name"] for file in results if file["role"] == "unlisted"],
               "active_records": sum(len(file["records"]) for file in active),
               "active_unique_opcodes": len(counts), "active_opcodes": dict(sorted(counts.items())),
               "active_opcode_payload_lengths": dict(sorted(payload_lengths.items())),
               "active_errors": [file["name"] for file in active if "error" in file],
               "unlisted_errors": [file["name"] for file in results if file["role"] == "unlisted" and "error" in file],
               "unlisted_with_base_map": unlisted_with_base_map}
    report.write_text(json.dumps({"format": 1, "summary": summary, "files": results}, indent=2) + "\n")
    print(f"Probed {summary['files']} EVT files, {summary['records']} records, "
          f"{summary['active_unique_opcodes']} opcodes in listed maps/global; "
          f"{len(summary['active_errors'])} malformed active files; "
          f"{len(summary['unlisted_errors'])} malformed unlisted files")
    print("Active opcode counts:", " ".join(f"{opcode}:{count}" for opcode, count in counts.most_common()))
    if summary["active_errors"] or summary["listed_map_files"] != len(listed_maps) or unlisted_with_base_map:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
