#!/usr/bin/env python3
"""Prepare ignored decoded MM6 indoor pairs for the native unit fixture.

Reads only F3-matched retained extracts, checks their frozen hashes and writes
decoded bytes beneath the same ignored research directory. No original game
installation or runtime is opened. The output contains proprietary game data.
"""

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
    probe = json.loads((root / "indoor-probe.json").read_text())
    expected = {row["name"].casefold(): row for row in probe["maps"] + probe["deltas"]}

    pairs = sorted((root / "Games").glob("*.blv"))
    if len(pairs) != 52:
        raise ValueError("expected 52 retained MM6 indoor base maps")
    prepared: list[tuple[Path, bytes]] = []
    for base in pairs:
        for path in (base, base.with_suffix(".dlv")):
            relative = path.relative_to(root).as_posix()
            row = signed.get(relative)
            if not row or not row["capture_stored_match"] or not row["decoded_matches_retained"]:
                raise ValueError("indoor member lacks F3 original identity")
            wrapper = path.read_bytes()
            if sha256(wrapper) != row["retained_sha256"]:
                raise ValueError("retained indoor member differs from F3")
            decoded, checksum_valid, _ = decode(path)
            prior = expected[path.name.casefold()]
            if (sha256(decoded) != row["decoded_sha256"] or
                    sha256(decoded) != prior["decoded_sha256"] or
                    checksum_valid != prior["checksum_valid"]):
                raise ValueError("decoded indoor member differs from F2 probe")
            prepared.append((path, decoded))

    output = root / "F4_INDOOR_DECODED"
    output.mkdir(exist_ok=True)
    for path, decoded in prepared:
        destination = output / path.name
        if destination.exists():
            if sha256(destination.read_bytes()) != sha256(decoded):
                raise ValueError("existing private fixture differs")
        else:
            destination.write_bytes(decoded)
    print(f"Prepared {len(pairs)} F3-matched indoor pairs in ignored local storage")
    print(output)


if __name__ == "__main__":
    main()
