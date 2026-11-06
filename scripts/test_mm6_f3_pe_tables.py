#!/usr/bin/env python3
"""Synthetic PE bounds and table-shape checks; no original game file read."""

import struct
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))

from mm6_f3_pe_tables import (HOUSE_SIZE, HOUSES_VA, PEImage, SPELL_SIZE,
                              SPELLS_VA, TRANSPORT_INDEX_VA, TRANSPORT_VA,
                              extract_tables)


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


if __name__ == "__main__":
    unittest.main()
