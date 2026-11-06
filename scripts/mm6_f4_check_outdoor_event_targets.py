#!/usr/bin/env python3
"""Check active MM6 outdoor texture/facet operands against retained map models.

Reads only ignored F3-matched extracts; prints aggregate evidence, never asset
bytes or map names. This does not establish the original event side effects.
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path

from mm6_probe_indoors import decode


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research_dir", type=Path)
    root = parser.parse_args().research_dir.resolve(strict=True)
    if root.parent.name != "mm6" or root.parent.parent.name != "local-game-data":
        parser.error("research directory must be inside ignored local-game-data/mm6")

    gate = json.loads((root / "F3_GATE_REPORT_HASHES_2026-11-06.json").read_text())
    content_path = root / "F3_CONTENT_RECONCILIATION_2026-11-05.json"
    if digest(content_path) != gate["reports_sha256"][content_path.name]:
        raise ValueError("F3 content report differs from gate")
    signed = {row["retained_path"]: row for row in json.loads(content_path.read_text())["files"]}
    events = json.loads((root / "event-probe.json").read_text())
    outdoors = {row["name"].casefold(): row for row in json.loads((root / "outdoor-probe.json").read_text())["maps"]}

    texture_count = facet_count = one_based_texture_valid = 0
    for event in events["files"]:
        if event["role"] != "listed_map":
            continue
        stem = Path(event["name"]).stem
        base = root / "Games" / (stem + ".odm")
        if not base.exists():
            continue
        event_path = root / "icons" / event["name"]
        for path in (event_path, base):
            relative = path.relative_to(root).as_posix()
            row = signed.get(relative)
            if not row or not row["capture_stored_match"] or digest(path) != row["retained_sha256"]:
                raise ValueError("outdoor event or map differs from F3 original match")

        raw = event_path.read_bytes()
        if hashlib.sha256(raw).hexdigest() != event["sha256"]:
            raise ValueError("outdoor EVT differs from F2 probe")
        decoded, _, _ = decode(base)
        report = outdoors[base.name.casefold()]
        if hashlib.sha256(decoded).hexdigest() != report["decoded_sha256"]:
            raise ValueError("outdoor map differs from F2 probe")
        offset = 160 + 16 + 3 * 128 * 128
        count = struct.unpack_from("<I", decoded, offset)[0]
        offset += 4
        faces = [struct.unpack_from("<I", decoded, offset + i * 188 + 76)[0] for i in range(count)]

        for record in event["records"]:
            if record["opcode"] not in (12, 24):
                continue
            start = record["offset"]
            payload = raw[start:start + record["size"]]
            model, facet = struct.unpack_from("<II", payload, 5)
            if model >= count:
                raise ValueError("outdoor event model is outside zero-based bounds")
            if record["opcode"] == 12:
                if facet >= faces[model]:
                    raise ValueError("outdoor texture facet is outside zero-based bounds")
                texture_count += 1
                one_based_texture_valid += int(model > 0 and 1 <= facet <= faces[model - 1])
            else:
                if facet != 0xFFFFFFFF:
                    raise ValueError("active outdoor facet event is not the observed all-faces sentinel")
                facet_count += 1

    if (texture_count, facet_count, one_based_texture_valid) != (12, 6, 8):
        raise ValueError("active outdoor event target counts differ from frozen evidence")
    print("F3-matched active outdoor targets: 12/12 zero-based textures, 6/6 all-faces facets")
    print("Only 8/12 textures also fit a one-based model interpretation")


if __name__ == "__main__":
    main()
