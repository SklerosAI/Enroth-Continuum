#!/usr/bin/env python3
"""Build the ignored 68-resource MM6 EVT parser fixture from frozen F2 evidence."""

import argparse
import hashlib
import json
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research_dir", type=Path, help="ignored MM6 research directory")
    args = parser.parse_args()

    root = args.research_dir
    report_path = root / "event-probe.json"
    freeze = json.loads((root / "F2_FREEZE_MANIFEST.json").read_text())
    key = report_path.as_posix()
    expected = freeze["ignored_derived_sha256"].get(key)
    if not expected or hashlib.sha256(report_path.read_bytes()).hexdigest() != expected:
        raise ValueError("event report does not match the F2 freeze manifest")

    report = json.loads(report_path.read_text())
    names = [row["name"] for row in report["files"] if row["role"] in {"listed_map", "global"}]
    if len(names) != 68 or len(set(names)) != 68:
        raise ValueError("expected 67 listed map EVT resources and one global resource")
    if any(Path(name).name != name or not name.lower().endswith(".evt") for name in names):
        raise ValueError("unexpected EVT member name in frozen report")

    output = root / "F4_ACTIVE_EVT_LIST_2026-11-06.txt"
    content = "\n".join(names) + "\n"
    if output.exists() and output.read_text() != content:
        raise ValueError("existing active EVT fixture differs; refusing overwrite")
    output.write_text(content)
    print(f"Private active EVT fixture verified: {len(names)} resources")


if __name__ == "__main__":
    main()
