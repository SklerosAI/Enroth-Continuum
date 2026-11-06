#!/usr/bin/env python3
"""Synthetic MM6 F3 snapshot checks; never opens owner game files."""

import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))

from mm6_f3_capture_snapshot import capture, digest, parse_lod, parse_snd, parse_vid


def lod(name: str = "sample.txt") -> bytes:
    raw = bytearray(324)
    raw[:4] = b"LOD\0"
    raw[4:8] = b"MM6\0"
    struct.pack_into("<I", raw, 172, 1)
    struct.pack_into("<16sIIIHH", raw, 256, b"root", 288, 36, 0, 1, 0)
    struct.pack_into("<16sIIIHH", raw, 288, name.encode(), 32, 4, 0, 0, 0)
    raw[320:] = b"data"
    return bytes(raw)


def vid(name: str = "movie") -> bytes:
    return struct.pack("<I40sI", 1, name.encode(), 48) + b"vid!"


def snd(name: str = "sound") -> bytes:
    return struct.pack("<I40sIII", 1, name.encode(), 56, 4, 4) + b"snd!"


class SnapshotTests(unittest.TestCase):
    def test_bounded_archive_inventory(self):
        self.assertEqual(parse_lod(lod())["entries"][0]["stored_sha256"], digest(b"data"))
        self.assertEqual(parse_vid(vid())["entries"][0]["stored_bytes"], 4)
        self.assertEqual(parse_snd(snd())["entries"][0]["decoded_bytes_field"], 4)
        with self.assertRaisesRegex(ValueError, "exceeds file"):
            parse_lod(lod()[:-1])
        broken = bytearray(vid())
        struct.pack_into("<I", broken, 44, 1000)
        with self.assertRaisesRegex(ValueError, "offset exceeds"):
            parse_vid(bytes(broken))

    def test_synthetic_eight_inputs_and_soundtrack_reconcile(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "original"
            ref = Path(directory) / "retained"
            (root / "data").mkdir(parents=True)
            (root / "Anims").mkdir()
            (root / "Sounds").mkdir()
            ref.mkdir()
            files = {
                "data/icons.lod": lod(), "data/Games.lod": lod(),
                "data/BITMAPS.LOD": lod(), "data/SPRITES.LOD": lod(),
                "Anims/Anims1.vid": vid(), "Anims/Anims2.vid": vid(),
                "Sounds/Audio.snd": snd(), "MM6.exe": b"synthetic exe",
            }
            for relative, data in files.items():
                path = root / relative
                path.write_bytes(data)
                if path.suffix.casefold() in {".lod", ".vid", ".snd"}:
                    (ref / f"{path.stem}.entries").write_text(
                        "sample.txt\n" if path.suffix.casefold() == ".lod"
                        else "movie\n" if path.suffix.casefold() == ".vid" else "sound\n")
            for ledger, relatives in (
                ("archive-sha256.txt", list(files)[:4]),
                ("media-executable-sha256.txt", list(files)[4:]),
            ):
                (ref / ledger).write_text("".join(
                    f"{digest(files[relative])}  /synthetic/app/{relative}\n" for relative in relatives))
            (ref / "catalog.json").write_text(json.dumps({"tables": {"sample.txt": {}}}))
            (ref / "map-probe.json").write_text(json.dumps({"maps": [{"name": "sample.txt"}]}))
            tracks = []
            for number in range(2, 17):
                data = f"track-{number}".encode()
                filename = f"{number}.mp3"
                (root / "Sounds" / filename).write_bytes(data)
                tracks.append({"file": filename, "bytes": len(data), "sha256": digest(data)})
            (ref / "music-manifest.json").write_text(json.dumps(tracks))
            result = capture(root, ref)
            self.assertEqual(result["status"], "pass")
            self.assertEqual((len(result["files"]), len(result["soundtrack"])), (8, 15))
            (root / "Sounds/2.mp3").write_bytes(b"changed")
            changed = capture(root, ref)
            self.assertEqual(changed["status"], "needs_reconciliation")
            self.assertEqual(len(changed["problems"]), 1)


if __name__ == "__main__":
    unittest.main()
