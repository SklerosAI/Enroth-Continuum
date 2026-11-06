#!/usr/bin/env python3
"""Prepare ignored decoded MM6 outdoor maps and deltas from F3-matched extracts."""

import argparse
import hashlib
import json
from pathlib import Path

from mm6_probe_indoors import decode


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research_dir", type=Path, help="ignored MM6 research directory")
    args = parser.parse_args()
    root = args.research_dir.resolve(strict=True)
    if root.parent.name != "mm6" or root.parent.parent.name != "local-game-data":
        parser.error("research directory must be inside ignored local-game-data/mm6")

    gate = json.loads((root / "F3_GATE_REPORT_HASHES_2026-11-06.json").read_text())
    content_path = root / "F3_CONTENT_RECONCILIATION_2026-11-05.json"
    if sha256(content_path.read_bytes()) != gate["reports_sha256"][content_path.name]:
        raise ValueError("F3 content report changed after signoff")
    content = json.loads(content_path.read_text())
    if content["status"] != "pass":
        raise ValueError("F3 content reconciliation did not pass")
    signed = {row["retained_path"]: row for row in content["files"] if row["archive"] == "Games"}
    probe = json.loads((root / "outdoor-probe.json").read_text())
    expected = {row["name"].casefold(): row for row in probe["maps"] + probe["deltas"]}

    maps = sorted((root / "Games").glob("*.odm"))
    if len(maps) != 15:
        raise ValueError("expected 15 retained MM6 outdoor base maps")
    prepared: list[tuple[Path, bytes]] = []
    for base in maps:
        for path in (base, base.with_suffix(".ddm")):
            relative = path.relative_to(root).as_posix()
            row = signed.get(relative)
            if not row or not row["capture_stored_match"] or not row["decoded_matches_retained"]:
                raise ValueError("outdoor member lacks F3 original identity")
            if sha256(path.read_bytes()) != row["retained_sha256"]:
                raise ValueError("retained outdoor member differs from F3")
            decoded, checksum_valid, _ = decode(path)
            prior = expected[path.name.casefold()]
            # F3's "decoded" field describes archive extraction; these retained
            # members still carry a separate map compression wrapper.
            if (sha256(decoded) != prior["decoded_sha256"] or
                    checksum_valid != prior["checksum_valid"]):
                raise ValueError("decoded outdoor member differs from F2 probe")
            prepared.append((path, decoded))

    output = root / "F4_OUTDOOR_DECODED"
    output.mkdir(exist_ok=True)
    for path, decoded in prepared:
        destination = output / path.name
        if destination.exists():
            if sha256(destination.read_bytes()) != sha256(decoded):
                raise ValueError("existing private fixture differs")
        else:
            destination.write_bytes(decoded)
    print(f"Prepared {len(maps)} F3-matched outdoor pairs in ignored local storage")
    print(output)


if __name__ == "__main__":
    main()
