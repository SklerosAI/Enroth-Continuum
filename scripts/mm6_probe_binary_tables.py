#!/usr/bin/env python3
"""Validate the record boundaries of retained MM6 binary tables offline.

Record sizes come from the fork's MM6 snapshot types and, for dchest.bin,
MM8LevelEditor's DChestBin reader:
https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/DChestBin.java
The ignored report contains metadata only.
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path


# dchest.bin is shipped but currently has no runtime consumer in this fork.
RECORD_SIZES = {
    "dchest.bin": 36,
    "ddeclist.bin": 80,
    "dift.bin": 32,
    "dmonlist.bin": 148,
    "dobjlist.bin": 52,
    "doverlay.bin": 8,
    "dpft.bin": 10,
    "dsounds.bin": 112,
    "dtft.bin": 20,
    "dtile.bin": 26,
}


def probe(path: Path) -> dict:
    data = path.read_bytes()
    entry = {"name": path.name, "bytes": len(data),
             "sha256": hashlib.sha256(data).hexdigest()}
    if path.name == "dsft.bin":
        if len(data) < 8:
            entry["error"] = "shorter than the two-count sprite-frame header"
            return entry
        frames, extended_frames = struct.unpack_from("<II", data)
        entry.update(frame_count=frames, extended_frame_count=extended_frames,
                     frame_record_bytes=56, extended_frame_record_bytes=2,
                     expected_bytes=8 + frames * 56 + extended_frames * 2)
    else:
        if len(data) < 4:
            entry["error"] = "shorter than the record-count header"
            return entry
        count = struct.unpack_from("<I", data)[0]
        record_size = RECORD_SIZES[path.name]
        entry.update(record_count=count, record_bytes=record_size,
                     expected_bytes=4 + count * record_size)
    if entry["expected_bytes"] != len(data):
        entry["error"] = "declared records do not consume the file exactly"
    elif path.name == "dchest.bin":
        layouts = []
        for index in range(entry["record_count"]):
            start = 4 + index * 36
            width, height = data[start + 32:start + 34]
            image_id = struct.unpack_from("<H", data, start + 34)[0]
            layouts.append({"index": index, "width": width, "height": height,
                            "image_id": image_id})
            if not 0 < width * height <= 140:
                entry["error"] = f"chest {index} exceeds the 140-cell inventory matrix"
        entry["chest_layouts"] = layouts
    return entry


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
    expected = set(RECORD_SIZES) | {"dsft.bin"}
    actual = {path.name for path in icons.glob("*.bin")}
    missing = sorted(expected - actual)
    unexpected = sorted(actual - expected)
    entries = [probe(icons / name) for name in sorted(actual & expected)]
    failures = [entry["name"] for entry in entries if "error" in entry]
    result = {"format": 1, "tables": entries,
              "summary": {"files": len(entries), "missing": missing,
                          "unexpected": unexpected, "boundary_failures": failures}}
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Checked {len(entries)} MM6 binary tables; {len(failures)} boundary failures")
    if missing or unexpected or failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
