#!/usr/bin/env python3
"""Synthetic PE bounds and table-shape checks; no original game file read."""

import struct
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))

from mm6_f3_pe_tables import (HOUSE_SIZE, HOUSES_VA, PEImage, SPELL_SIZE,
                              SPELLS_VA, TRANSPORT_INDEX_VA, TRANSPORT_VA,
                              extract_tables)
from mm6_f3_identity_preflight import sha256 as file_sha256, verify_frozen_inputs


def synthetic_pe() -> bytearray:
    raw = bytearray(0x15200)
    raw[:2] = b"MZ"
    struct.pack_into("<I", raw, 0x3C, 0x80)
    raw[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", raw, 0x86, 1)
    struct.pack_into("<H", raw, 0x94, 0xE0)
    struct.pack_into("<H", raw, 0x98, 0x10B)
    struct.pack_into("<I", raw, 0x98 + 28, 0x400000)
    section = 0x178
    raw[section:section + 8] = b".data\0\0\0"
    struct.pack_into("<IIII", raw, section + 8, 0x15000, 0xB0000, 0x15000, 0x200)

    def offset(va: int) -> int:
        return 0x200 + va - 0x4B0000

    raw[offset(0x4C2630):offset(0x4C2630) + 6] = bytes([10, 11, 12, 13, 14, 15])
    raw[offset(0x4C2694):offset(0x4C2694) + 4] = bytes([0, 1, 2, 3])
    struct.pack_into("<13h", raw, offset(0x4C2750), *range(13))
    struct.pack_into("<hhhhhhH", raw, offset(SPELLS_VA + SPELL_SIZE),
                     1, 2, 3, 70, 80, 90, 0)
    room = offset(HOUSES_VA + HOUSE_SIZE)
    struct.pack_into("<I", raw, room, 0x4C2000)
    raw[room + 4] = 7
    struct.pack_into("<i", raw, room + 8, 42)
    raw[room + 12:room + 14] = bytes([5, 9])
    raw[offset(0x4C2000):offset(0x4C2000) + 9] = b"roomname\0"
    raw[offset(TRANSPORT_VA):offset(TRANSPORT_INDEX_VA)] = bytes([0x5A]) * (
        TRANSPORT_INDEX_VA - TRANSPORT_VA)
    return raw


class PETablesTests(unittest.TestCase):
    def test_candidate_tables_are_bounded_and_mapped(self):
        image = PEImage(bytes(synthetic_pe()))
        result = extract_tables(image)
        self.assertEqual(result["creation"]["hp_base"]["values"], [10, 11, 12, 13, 14, 15])
        self.assertEqual(result["creation"]["starting_skill_codes_outside_0_3"], 0)
        self.assertEqual(result["creation"]["skill_recovery"]["values"][12], 12)
        self.assertEqual(result["spells"]["rows"][1]["delay"], [70, 80, 90])
        self.assertEqual(result["house_movies"]["rows"][1]["file_name"]["value"], "roomname")
        self.assertEqual(result["house_movies"]["rows"][1]["sound_group"], 9)
        self.assertEqual(len(result["transport_candidate"]["record_hex"]), 36)
        self.assertEqual(result["transport_candidate"]["candidate_record_bytes"], 32)

    def test_malformed_headers_and_unbacked_spans_fail(self):
        with self.assertRaises(ValueError):
            PEImage(bytes(synthetic_pe()[:0x100]))
        image = PEImage(bytes(synthetic_pe()))
        with self.assertRaises(ValueError):
            image.file_offset(0x3FFFFF, 1)
        with self.assertRaises(ValueError):
            image.file_offset(0x4C4FF0, 100)

    def test_one_unmapped_candidate_preserves_other_tables(self):
        raw = synthetic_pe()
        struct.pack_into("<I", raw, 0x178 + 16, 0x14000)
        result = extract_tables(PEImage(bytes(raw)))
        self.assertIn("transport_candidate", result["errors"])
        self.assertIn("spells", result)
        self.assertIn("creation", result)


class FrozenInputTests(unittest.TestCase):
    def test_hashes_match_then_reject_changed_private_report(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            tracked = repo / "project/MM6_F2_GATE_REVIEW.md"
            report = repo / "local-game-data/mm6/research/f2-report.json"
            tracked.parent.mkdir(parents=True)
            report.parent.mkdir(parents=True)
            tracked.write_text("frozen specification\n")
            report.write_text('{"count": 1}\n')
            manifest = {
                "tracked_sha256": {tracked.relative_to(repo).as_posix(): file_sha256(tracked)},
                "ignored_derived_sha256": {report.relative_to(repo).as_posix(): file_sha256(report)},
            }
            verify_frozen_inputs(repo, report.parent, manifest)
            report.write_text('{"count": 2}\n')
            with self.assertRaisesRegex(ValueError, "changed frozen ignored derived input"):
                verify_frozen_inputs(repo, report.parent, manifest)

    def test_manifest_rejects_bad_hash_type_and_path_escape(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            reference = repo / "local-game-data/mm6/research"
            reference.mkdir(parents=True)
            manifest = {"tracked_sha256": {"project/foo.md": 42},
                        "ignored_derived_sha256": {"local-game-data/mm6/research/foo.json": "0" * 64}}
            with self.assertRaisesRegex(ValueError, "invalid tracked manifest entry"):
                verify_frozen_inputs(repo, reference, manifest)
            manifest["tracked_sha256"] = {"../outside.md": "0" * 64}
            with self.assertRaisesRegex(ValueError, "unsafe tracked manifest path"):
                verify_frozen_inputs(repo, reference, manifest)


if __name__ == "__main__":
    unittest.main()
