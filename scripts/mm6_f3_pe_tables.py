#!/usr/bin/env python3
"""F3-only read-only extraction of candidate MM6 executable tables.

Use only after the F2 freeze and eight-file identity preflight have passed.
The PE addresses come from a pinned MMExtension MM6 source revision; a source
address is not a validated file offset or proof of original runtime effects.
All extracted values and pointer targets stay in an ignored private report.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from mm6_f3_identity_preflight import expected_files


SPELLS_VA = 0x4BDD70
SPELL_COUNT = 100
SPELL_SIZE = 14
HOUSES_VA = 0x4BE888
HOUSE_COUNT = 119
HOUSE_SIZE = 16
TRANSPORT_VA = 0x4C3F20
TRANSPORT_INDEX_VA = 0x4C43A0
TRANSPORT_COUNT = 36
TRANSPORT_CANDIDATE_SIZE = (TRANSPORT_INDEX_VA - TRANSPORT_VA) // TRANSPORT_COUNT
CREATION_U8_FIELDS = {
    "hp_base": (0x4C2630, 6), "sp_base": (0x4C2638, 6),
    "hp_factor": (0x4C2640, 18), "sp_factor": (0x4C2654, 18),
    "starting_stats": (0x4C2668, 6 * 7),
    "starting_skills": (0x4C2694, 6 * 31),
}
SKILL_RECOVERY_VA = 0x4C2750
SKILL_RECOVERY_COUNT = 13


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class PEImage:
    def __init__(self, data: bytes):
        self.data = data
        if len(data) < 0x40 or data[:2] != b"MZ":
            raise ValueError("not a bounded MZ executable")
        pe_at = struct.unpack_from("<I", data, 0x3C)[0]
        if pe_at > len(data) - 24 or data[pe_at:pe_at + 4] != b"PE\0\0":
            raise ValueError("missing or out-of-bounds PE signature")
        section_count = struct.unpack_from("<H", data, pe_at + 6)[0]
        optional_size = struct.unpack_from("<H", data, pe_at + 20)[0]
        optional_at = pe_at + 24
        section_at = optional_at + optional_size
        if not 1 <= section_count <= 96 or optional_size < 0x60:
            raise ValueError("unexpected PE section count or optional-header size")
        if section_at + section_count * 40 > len(data):
            raise ValueError("PE section headers exceed file")
        if struct.unpack_from("<H", data, optional_at)[0] != 0x10B:
            raise ValueError("expected a 32-bit PE optional header")
        self.image_base = struct.unpack_from("<I", data, optional_at + 28)[0]
        self.sections = []
        for index in range(section_count):
            at = section_at + index * 40
            name = data[at:at + 8].split(b"\0", 1)[0].decode("ascii", errors="replace")
            virtual_size, rva, raw_size, raw_at = struct.unpack_from("<IIII", data, at + 8)
            if raw_at > len(data) or raw_size > len(data) - raw_at:
                raise ValueError(f"PE section {name} raw extent exceeds file")
            self.sections.append({"name": name, "virtual_size": virtual_size,
                                  "rva": rva, "raw_size": raw_size, "raw_at": raw_at})

    def file_offset(self, va: int, length: int) -> tuple[int, str]:
        if length < 0 or va < self.image_base:
            raise ValueError("virtual address below image base or negative length")
        rva = va - self.image_base
        matches = []
        for section in self.sections:
            distance = rva - section["rva"]
            virtual_limit = section["virtual_size"] or section["raw_size"]
            if (0 <= distance and distance + length <= section["raw_size"]
                    and distance + length <= virtual_limit):
                matches.append((section["raw_at"] + distance, section["name"]))
        if len(matches) != 1:
            raise ValueError(f"virtual span 0x{va:X}+{length} has {len(matches)} file-backed PE mappings")
        return matches[0]

    def slice(self, va: int, length: int) -> tuple[bytes, int, str]:
        offset, section = self.file_offset(va, length)
        return self.data[offset:offset + length], offset, section

    def c_string(self, va: int, limit: int = 128) -> dict:
        if va == 0:
            return {"pointer": 0, "status": "null", "value": ""}
        try:
            offset, section = self.file_offset(va, 1)
        except ValueError:
            return {"pointer": va, "status": "not_file_backed"}
        section_data = next(row for row in self.sections if row["name"] == section)
        end_bound = min(offset + limit, section_data["raw_at"] + section_data["raw_size"])
        end = self.data.find(b"\0", offset, end_bound)
        if end < 0:
            return {"pointer": va, "status": "unterminated_or_long", "file_offset": offset}
        return {"pointer": va, "status": "file_backed", "file_offset": offset,
                "value": self.data[offset:end].decode("cp1252", errors="replace")}


def extract_creation(image: PEImage) -> dict:
    creation = {}
    for label, (va, count) in CREATION_U8_FIELDS.items():
        raw, at, section = image.slice(va, count)
        creation[label] = {"va": va, "file_offset": at, "section": section,
                           "values": list(raw)}
    raw, at, section = image.slice(SKILL_RECOVERY_VA, SKILL_RECOVERY_COUNT * 2)
    creation["skill_recovery"] = {"va": SKILL_RECOVERY_VA,
                                  "file_offset": at, "section": section,
                                  "values": list(struct.unpack("<13h", raw))}
    creation["starting_skill_codes_outside_0_3"] = sum(
        value > 3 for value in creation["starting_skills"]["values"])
    return creation


def extract_spells(image: PEImage) -> dict:
    spells_raw, spells_at, spells_section = image.slice(SPELLS_VA, SPELL_COUNT * SPELL_SIZE)
    spells = []
    for number in range(SPELL_COUNT):
        at = number * SPELL_SIZE
        points = list(struct.unpack_from("<hhh", spells_raw, at))
        delays = list(struct.unpack_from("<hhh", spells_raw, at + 6))
        bits = struct.unpack_from("<H", spells_raw, at + 12)[0]
        spells.append({"id": number, "spell_points": points, "delay": delays, "bits": bits})
    return {"va": SPELLS_VA, "file_offset": spells_at, "section": spells_section,
            "record_bytes": SPELL_SIZE, "rows": spells}


def extract_houses(image: PEImage) -> dict:
    houses_raw, houses_at, houses_section = image.slice(HOUSES_VA, HOUSE_COUNT * HOUSE_SIZE)
    houses = []
    for number in range(HOUSE_COUNT):
        at = number * HOUSE_SIZE
        filename_va = struct.unpack_from("<I", houses_raw, at)[0]
        houses.append({"id": number, "file_name": image.c_string(filename_va),
                       "background": houses_raw[at + 4],
                       "npc_picture": struct.unpack_from("<i", houses_raw, at + 8)[0],
                       "house_type": houses_raw[at + 12], "sound_group": houses_raw[at + 13]})
    return {"va": HOUSES_VA, "file_offset": houses_at,
            "section": houses_section, "record_bytes": HOUSE_SIZE, "rows": houses}


def extract_transport(image: PEImage) -> dict:
    # MMExtension gives the next table address, not a TravelInfo record width.
    # Preserve the intervening bytes as an opaque candidate until F3 decodes it.
    if (TRANSPORT_INDEX_VA - TRANSPORT_VA) % TRANSPORT_COUNT:
        raise ValueError("transport candidate span does not divide into 36 records")
    transport_raw, transport_at, transport_section = image.slice(
        TRANSPORT_VA, TRANSPORT_INDEX_VA - TRANSPORT_VA)
    transports = [transport_raw[i:i + TRANSPORT_CANDIDATE_SIZE].hex()
                  for i in range(0, len(transport_raw), TRANSPORT_CANDIDATE_SIZE)]
    return {"va": TRANSPORT_VA, "file_offset": transport_at,
            "section": transport_section,
            "candidate_record_bytes": TRANSPORT_CANDIDATE_SIZE,
            "record_hex": transports}


def extract_tables(image: PEImage) -> dict:
    tables = {}
    errors = {}
    for label, extractor in (("creation", extract_creation), ("spells", extract_spells),
                             ("house_movies", extract_houses),
                             ("transport_candidate", extract_transport)):
        try:
            tables[label] = extractor(image)
        except (ValueError, struct.error) as exc:
            errors[label] = str(exc)
    tables["errors"] = errors
    return tables


def gate_check(repo: Path, reference: Path, identity_report: Path) -> str:
    gate = repo / "project/MM6_F2_GATE_REVIEW.md"
    if not any(line.startswith("Status: **frozen") for line in gate.read_text().splitlines()[:10]):
        raise ValueError("F2 gate is not frozen; original executable read is unavailable")
    manifest_path = reference / "F2_FREEZE_MANIFEST.json"
    if not manifest_path.is_file():
        raise ValueError("missing ignored F2 freeze manifest")
    manifest = json.loads(manifest_path.read_text())
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip()
    if manifest.get("source_commit") != commit:
        raise ValueError("source commit differs from the F2 freeze manifest")
    if subprocess.check_output(["git", "status", "--porcelain", "--untracked-files=all"],
                               cwd=repo, text=True).strip():
        raise ValueError("tracked worktree must be clean before original executable read")
    if not identity_report.is_file() or not identity_report.is_relative_to(reference):
        raise ValueError("missing ignored F3 identity report")
    identity = json.loads(identity_report.read_text())
    if identity.get("summary") != {"match": 8, "mismatch": 0, "missing_or_symlink": 0}:
        raise ValueError("eight-file F3 identity check has not passed")
    matching = [row for row in identity.get("files", [])
                if row.get("relative_path", "").casefold() == "mm6.exe" and row.get("status") == "match"]
    if len(matching) != 1:
        raise ValueError("identity report lacks one matched MM6 executable")
    entries, _ = expected_files(reference)
    ledger = [row for row in entries if row["relative_path"].casefold() == "mm6.exe"]
    if len(ledger) != 1 or matching[0].get("expected_sha256") != ledger[0]["expected_sha256"]:
        raise ValueError("executable identity differs from retained F1 ledger")
    return ledger[0]["expected_sha256"]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_exe", type=Path)
    parser.add_argument("reference", type=Path)
    parser.add_argument("identity_report", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    reference = args.reference.resolve(strict=True)
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    output = args.report.resolve()
    identity_report = args.identity_report.resolve()
    if reference == allowed or not reference.is_relative_to(allowed):
        parser.error("reference must stay in ignored local-game-data/mm6")
    if not output.is_relative_to(reference) or output.suffix.lower() != ".json" or not output.parent.is_dir():
        parser.error("report must be JSON inside ignored reference directory")
    if output.exists():
        parser.error("F3 PE-table report already exists; preserve the first extraction")
    if subprocess.run(["git", "check-ignore", "-q", str(output)], cwd=repo).returncode != 0:
        parser.error("F3 PE-table report path must be Git ignored")
    try:
        expected = gate_check(repo, reference, identity_report)
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        parser.error(str(exc))
    if args.original_exe.is_symlink():
        parser.error("original executable must not be a symlink")
    original = args.original_exe.resolve(strict=True)
    if not original.is_file() or original.is_relative_to(reference) or original.name.casefold() != "mm6.exe":
        parser.error("expected a separate original MM6.exe file")
    data = original.read_bytes()
    actual = sha256(data)
    if actual != expected:
        output.write_text(json.dumps({"format": 1, "status": "identity_mismatch",
                                      "expected_sha256": expected, "actual_sha256": actual}, indent=2) + "\n")
        parser.error("original executable hash differs from retained F1 identity; private report retained")
    try:
        image = PEImage(data)
    except ValueError as exc:
        output.write_text(json.dumps({"format": 1, "status": "pe_parse_failed",
                                      "exe_sha256": actual, "error": str(exc)}, indent=2) + "\n")
        parser.error("PE header validation failed; private report retained")
    tables = extract_tables(image)
    payload = {"format": 1, "scope": "F3 static PE candidate tables; original runtime behavior unverified",
               "exe_sha256": actual, "image_base": image.image_base,
               "sections": image.sections, "status": "complete" if not tables["errors"] else "partial",
               "tables": tables}
    output.write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps({"exe_identity": "match", "table_status": payload["status"],
                      "creation_fields": len(tables.get("creation", {})),
                      "starting_skill_codes_outside_0_3": tables.get("creation", {}).get("starting_skill_codes_outside_0_3"),
                      "spell_rows": len(tables.get("spells", {}).get("rows", [])),
                      "house_rows": len(tables.get("house_movies", {}).get("rows", [])),
                      "transport_candidate_rows": len(tables.get("transport_candidate", {}).get("record_hex", [])),
                      "house_file_backed_names": sum(row["file_name"]["status"] == "file_backed"
                                                     for row in tables.get("house_movies", {}).get("rows", []))}))
    if tables["errors"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
