#!/usr/bin/env python3
"""Check MM6 missile object/frame/archive joins in ignored retained data."""

import argparse
import struct
from pathlib import Path


PROJECTILES = (
    (500, "msl01", 3), (510, "msl02", 3), (520, "msl03", 6),
    (530, "msl04", 6), (540, "msl05", 6), (550, "msl06", 6),
    (560, "msl07", 4), (570, "msl08", 6), (580, "msl09", 6),
)


def check(research: Path) -> None:
    icons = research / "icons"
    objects = (icons / "dobjlist.bin").read_bytes()
    frames = (icons / "dsft.bin").read_bytes()
    sprites = set((research / "SPRITES.entries").read_text().lower().splitlines())
    object_count = struct.unpack_from("<I", objects)[0]
    frame_count, eframe_count = struct.unpack_from("<II", frames)
    assert object_count == 232 and len(objects) == 4 + 52 * object_count
    assert frame_count == 6455 and len(frames) == 8 + 56 * frame_count + 2 * eframe_count

    by_id = {}
    for index in range(object_count):
        offset = 4 + 52 * index
        object_id = struct.unpack_from("<h", objects, offset + 32)[0]
        frame_id = struct.unpack_from("<H", objects, offset + 40)[0]
        assert object_id not in by_id, f"duplicate object ID {object_id}"
        by_id[object_id] = frame_id

    checked = 0
    for object_id, expected_name, expected_length in PROJECTILES:
        assert object_id in by_id, f"missing projectile object {object_id}"
        frame_id = by_id[object_id]
        assert 0 < frame_id < frame_count
        sequence_length = 0
        while True:
            assert frame_id + sequence_length < frame_count
            offset = 8 + 56 * (frame_id + sequence_length)
            row = frames[offset:offset + 56]
            name = row[:12].split(b"\0")[0].decode("ascii").lower()
            texture = row[12:24].split(b"\0")[0].decode("ascii").lower()
            flags = struct.unpack_from("<I", row, 44)[0]
            if sequence_length == 0:
                assert name == expected_name
            else:
                assert not name, f"{expected_name}: unexpected new sequence at {frame_id + sequence_length}"
            assert texture and any(entry.startswith(texture) for entry in sprites), (
                f"{expected_name}: missing sprite archive entry for {texture}"
            )
            sequence_length += 1
            if not flags & 1:
                break
        assert sequence_length == expected_length, (expected_name, sequence_length)
        checked += sequence_length

    print(f"MM6 projectile frames: 9 object joins, {checked} sequence frames, all archive names present")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research", type=Path, help="ignored F3-matched MM6 research directory")
    args = parser.parse_args()
    check(args.research)


if __name__ == "__main__":
    main()
