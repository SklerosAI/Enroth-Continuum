#!/usr/bin/env python3
"""Check retained MM6 media-name and candidate portrait-frame families.

Read only ignored F1 extracts. Keep exact resource/group names in an ignored
JSON report; print aggregate counts only. This does not open VID/SND originals.
"""

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def names(path: Path) -> list[str]:
    result = [line.strip() for line in path.read_text(encoding="cp1252").splitlines() if line.strip()]
    if len(result) != len(set(value.casefold() for value in result)):
        raise ValueError(f"case-insensitive duplicate in retained index {path.name}")
    return result


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as stream:
        header = stream.read(24)
    if len(header) != 24 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        raise ValueError(f"not a PNG header: {path.name}")
    return struct.unpack_from(">II", header, 16)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    output = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must stay inside ignored local-game-data/mm6")
    if not output.is_relative_to(root) or output.suffix.lower() != ".json" or not output.parent.is_dir():
        parser.error("JSON report must stay inside the ignored reference directory")

    index_paths = {key: root / (key + ".entries") for key in ("Anims1", "Anims2", "Audio", "icons")}
    indexes = {key: names(path) for key, path in index_paths.items()}
    if (len(indexes["Anims1"]), len(indexes["Anims2"]), len(indexes["Audio"])) != (40, 87, 1526):
        raise ValueError("retained media index counts changed")
    video = indexes["Anims1"] + indexes["Anims2"]
    if any("." in value for value in video + indexes["Audio"]):
        raise ValueError("retained VID/SND names are no longer all extensionless")

    groups: dict[str, list[str]] = defaultdict(list)
    for value in indexes["icons"]:
        if re.fullmatch(r"[A-Za-z]{5}[0-9]{2}", value):
            groups[value[:5].casefold()].append(value)
    candidate = {}
    dimensions = Counter()
    for prefix, members in sorted(groups.items()):
        frame_numbers = {int(value[-2:]) for value in members}
        if len(members) != 53 or frame_numbers != set(range(1, 54)):
            continue
        rows = []
        for value in sorted(members, key=lambda item: int(item[-2:])):
            path = root / "icons" / (value + ".png")
            if not path.is_file():
                raise ValueError(f"missing retained candidate PNG for group {prefix}")
            size = png_dimensions(path)
            dimensions[size] += 1
            rows.append({"name": value, "width": size[0], "height": size[1]})
        candidate[prefix] = rows
    if len(candidate) != 12 or dimensions != Counter({(59, 79): 636}):
        raise ValueError("candidate 12×53 portrait-shape invariant changed")

    dpft = root / "icons/dpft.bin"
    data = dpft.read_bytes()
    frame_count = struct.unpack_from("<I", data)[0] if len(data) >= 4 else 0
    if frame_count != 67 or len(data) != 4 + frame_count * 10:
        raise ValueError("retained portrait frame table boundary changed")
    frames = [struct.unpack_from("<HHhhh", data, 4 + i * 10) for i in range(frame_count)]
    texture_indices = {row[1] for row in frames}
    if min(texture_indices) != 1 or max(texture_indices) != 53:
        raise ValueError("portrait-frame texture-index span changed")

    summary = {
        "retained_video_entries": len(video),
        "retained_video_names_with_extension": 0,
        "retained_sound_entries": len(indexes["Audio"]),
        "retained_sound_names_with_extension": 0,
        "candidate_icon_frame_groups": len(candidate),
        "candidate_icon_frames": sum(map(len, candidate.values())),
        "candidate_frame_png_width": 59,
        "candidate_frame_png_height": 79,
        "portrait_frame_table_records": frame_count,
        "portrait_frame_table_unique_texture_indices": len(texture_indices),
        "portrait_frame_table_min_texture_index": min(texture_indices),
        "portrait_frame_table_max_texture_index": max(texture_indices),
    }
    payload = {
        "format": 1,
        "scope": "retained F1 entry indexes and extracted PNGs only; candidate family shape, not portrait identity",
        "summary": summary,
        "candidate_groups": candidate,
        "portrait_frame_records": [list(row) for row in frames],
        "input_sha256": {str(path.relative_to(root)): sha256(path) for path in [*index_paths.values(), dpft]},
    }
    output.write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    main()
