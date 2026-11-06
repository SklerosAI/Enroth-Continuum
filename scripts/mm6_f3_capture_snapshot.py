#!/usr/bin/env python3
"""F3-only bounded inventory of the owner's original MM6 files.

Run after the F2 freeze and 23-file identity check, in the same planned F3
session as the executable-table and content reconciliation. The output stays
under ignored local-game-data. This captures stored archive/media entry names,
bounds and hashes, plus all numbered soundtrack hashes. It does not decode
gameplay records, render media, or declare F3 complete.
"""

from collections import Counter
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from mm6_f3_identity_preflight import expected_files
from mm6_f3_pe_tables import gate_check


MAX_ENTRIES = 100_000
LOD_RECORD = struct.Struct("<16sIIIHH")
VID_RECORD = struct.Struct("<40sI")
SND_RECORD = struct.Struct("<40sIII")


def digest(data: bytes | memoryview) -> str:
    return hashlib.sha256(data).hexdigest()


def name(raw: bytes) -> str:
    value = raw.split(b"\0", 1)[0].decode("cp1252")
    if not value or any(ord(char) < 32 for char in value):
        raise ValueError("empty or control-character archive entry name")
    return value


def bounded_count(data: bytes, record: struct.Struct, start: int, count: int) -> int:
    if count < 0 or count > MAX_ENTRIES or start < 0 or start > len(data):
        raise ValueError("archive directory count or start exceeds bounds")
    end = start + count * record.size
    if end > len(data):
        raise ValueError("archive directory exceeds file")
    return end


def duplicate_names(rows: list[dict]) -> list[str]:
    counts = Counter(row["name"].casefold() for row in rows)
    return sorted(key for key, count in counts.items() if count > 1)


def parse_lod(data: bytes) -> dict:
    if len(data) < 288 or data[:4] != b"LOD\0":
        raise ValueError("missing MM6 LOD header")
    directories = struct.unpack_from("<I", data, 172)[0]
    if directories != 1:
        raise ValueError(f"expected one MM6 LOD directory, found {directories}")
    version = data[4:84].split(b"\0", 1)[0].decode("cp1252")
    root_name, root_at, root_size, _, count, _ = LOD_RECORD.unpack_from(data, 256)
    directory_end = bounded_count(data, LOD_RECORD, root_at, count)
    if root_at < 288:
        raise ValueError("LOD directory overlaps header")
    rows = []
    for index in range(count):
        raw_name, relative_at, size, _, children, priority = LOD_RECORD.unpack_from(
            data, root_at + index * LOD_RECORD.size)
        if children:
            raise ValueError("nested MM6 LOD directory is unsupported")
        start = root_at + relative_at
        end = start + size
        if (size and start < directory_end) or end > len(data):
            raise ValueError(f"LOD entry {index} overlaps directory or exceeds file")
        rows.append({"index": index, "name": name(raw_name), "offset": start,
                     "stored_bytes": size, "stored_sha256": digest(memoryview(data)[start:end]),
                     "within_declared_root": end <= root_at + root_size,
                     "priority": priority})
    return {"format": "lod-mm6", "version": version,
            "root_name": root_name.split(b"\0", 1)[0].decode("cp1252"),
            "root_offset": root_at, "root_declared_bytes": root_size,
            "root_declared_fits_file": root_at + root_size <= len(data),
            "entry_count": len(rows), "duplicate_casefold_names": duplicate_names(rows),
            "entries": rows}


def parse_vid(data: bytes) -> dict:
    if len(data) < 4:
        raise ValueError("VID lacks count")
    count = struct.unpack_from("<I", data)[0]
    directory_end = bounded_count(data, VID_RECORD, 4, count)
    rows = []
    for index in range(count):
        raw_name, start = VID_RECORD.unpack_from(data, 4 + index * VID_RECORD.size)
        if start < directory_end or start > len(data):
            raise ValueError(f"VID entry {index} offset exceeds payload bounds")
        rows.append({"index": index, "name": name(raw_name), "offset": start})
    sorted_rows = sorted(rows, key=lambda row: row["offset"])
    for index, row in enumerate(sorted_rows):
        end = sorted_rows[index + 1]["offset"] if index + 1 < count else len(data)
        row["stored_bytes"] = end - row["offset"]
        row["stored_sha256"] = digest(memoryview(data)[row["offset"]:end])
    return {"format": "vid-44", "entry_count": count,
            "duplicate_casefold_names": duplicate_names(rows),
            "last_region_tail_u32": (struct.unpack_from("<I", data, len(data) - 4)[0]
                                     if len(data) >= directory_end + 4 else None),
            "entries": rows}


def parse_snd(data: bytes) -> dict:
    if len(data) < 4:
        raise ValueError("SND lacks count")
    count = struct.unpack_from("<I", data)[0]
    directory_end = bounded_count(data, SND_RECORD, 4, count)
    rows = []
    for index in range(count):
        raw_name, start, size, decoded_size = SND_RECORD.unpack_from(
            data, 4 + index * SND_RECORD.size)
        end = start + size
        if (size and start < directory_end) or end > len(data):
            raise ValueError(f"SND entry {index} exceeds payload bounds")
        rows.append({"index": index, "name": name(raw_name), "offset": start,
                     "stored_bytes": size, "decoded_bytes_field": decoded_size,
                     "stored_sha256": digest(memoryview(data)[start:end])})
    return {"format": "snd-52", "entry_count": count,
            "duplicate_casefold_names": duplicate_names(rows), "entries": rows}


def compare_names(rows: list[dict], retained: Path) -> dict:
    expected = [line.strip() for line in retained.read_text(encoding="cp1252").splitlines()
                if line.strip()]
    actual_names = [row["name"] for row in rows]
    actual = Counter(value.casefold() for value in actual_names)
    frozen = Counter(value.casefold() for value in expected)
    return {"retained_index_sha256": digest(retained.read_bytes()),
            "retained_count": len(expected), "original_count": len(rows),
            "casefold_multiset_match": actual == frozen,
            "missing_from_original": list((frozen - actual).elements()),
            "extra_in_original": list((actual - frozen).elements()),
            "same_order_and_case": actual_names == expected}


def original_file(root: Path, relative: str) -> Path:
    root = root.resolve()
    path = root.joinpath(*Path(relative).parts)
    if path.is_symlink() or not path.is_file() or not path.resolve().is_relative_to(root):
        raise ValueError(f"missing, symlinked or escaped original input: {relative}")
    return path


def capture(root: Path, reference: Path) -> dict:
    entries, ledger_hashes = expected_files(reference)
    catalog = json.loads((reference / "catalog.json").read_text(encoding="utf-8"))
    map_probe = json.loads((reference / "map-probe.json").read_text(encoding="utf-8"))
    if (not isinstance(catalog.get("tables"), dict)
            or not isinstance(map_probe.get("maps"), list)):
        raise ValueError("retained table/map manifests missing or malformed")
    soundtrack = json.loads((reference / "music-manifest.json").read_text(encoding="utf-8"))
    if not isinstance(soundtrack, list) or len(soundtrack) != 15:
        raise ValueError("retained 15-track music manifest missing or malformed")
    for record in soundtrack:
        if not isinstance(record, dict):
            raise ValueError("retained music manifest has a malformed row")
        filename = record.get("file")
        if (not isinstance(filename, str) or not filename[:-4].isdigit()
                or filename[-4:].casefold() != ".mp3"):
            raise ValueError("retained music manifest has an unsafe filename")
    candidates = [path for path in root.iterdir() if path.is_dir() and path.name.casefold() == "sounds"]
    if len(candidates) != 1 or candidates[0].is_symlink():
        raise ValueError("original Sounds directory missing or ambiguous")
    sounds = candidates[0]
    results = []
    problems = []
    retained_track_names = {row["file"].casefold() for row in soundtrack}
    original_track_names = {path.name.casefold() for path in sounds.iterdir()
                            if path.is_file() and path.suffix.casefold() == ".mp3"}
    extra_tracks = sorted(original_track_names - retained_track_names)
    missing_tracks = sorted(retained_track_names - original_track_names)
    if extra_tracks or missing_tracks:
        problems.append("original soundtrack filenames differ from retained manifest")
    for expected in entries:
        relative = expected["relative_path"]
        try:
            path = original_file(root, relative)
            data = path.read_bytes()
        except OSError as exc:
            results.append({"relative_path": relative, "error": type(exc).__name__})
            problems.append(f"original file unreadable: {relative}")
            continue
        except ValueError as exc:
            results.append({"relative_path": relative, "error": str(exc)})
            problems.append(f"original file missing or unsafe: {relative}")
            continue
        row = {"relative_path": relative, "bytes": len(data), "sha256": digest(data),
               "expected_sha256": expected["expected_sha256"],
               "identity_match": digest(data) == expected["expected_sha256"]}
        if not row["identity_match"]:
            problems.append(f"original file identity mismatch: {relative}")
        suffix = path.suffix.casefold()
        parser = {".lod": parse_lod, ".vid": parse_vid, ".snd": parse_snd}.get(suffix)
        if parser and row["identity_match"]:
            try:
                row["inventory"] = parser(data)
                stem = path.stem
                retained_index = reference / (stem + ".entries")
                if retained_index.is_file():
                    row["retained_names"] = compare_names(row["inventory"]["entries"], retained_index)
                    if not row["retained_names"]["casefold_multiset_match"]:
                        problems.append(f"retained entry-name mismatch: {relative}")
                else:
                    problems.append(f"missing retained entry index: {stem}")
            except (ValueError, OSError) as exc:
                row["inventory_error"] = str(exc)
                problems.append(f"archive inventory failed: {relative}")
        results.append(row)
        del data

    tracks = []
    for record in soundtrack:
        filename = record.get("file")
        try:
            path = original_file(sounds, filename)
            data = path.read_bytes()
        except (OSError, ValueError) as exc:
            tracks.append({"file": filename, "error": str(exc)})
            problems.append(f"soundtrack missing or unreadable: {filename}")
            continue
        row = {"file": filename, "bytes": len(data), "sha256": digest(data),
               "retained_bytes": record.get("bytes"), "retained_sha256": record.get("sha256")}
        row["retained_match"] = row["bytes"] == row["retained_bytes"] and row["sha256"] == row["retained_sha256"]
        if not row["retained_match"]:
            problems.append(f"soundtrack mismatch: {filename}")
        tracks.append(row)
    lod_rows = [row for row in results if row.get("inventory", {}).get("format") == "lod-mm6"]
    all_lod_names = {member["name"].casefold()
                     for row in lod_rows for member in row["inventory"]["entries"]}
    expected_tables = {value.casefold() for value in catalog["tables"]}
    actual_games = {member["name"].casefold()
                    for row in lod_rows if Path(row["relative_path"]).stem.casefold() == "games"
                    for member in row["inventory"]["entries"]}
    expected_maps = {row["name"].casefold() for row in map_probe["maps"]}
    families = {
        "text_tables": {"retained_count": len(expected_tables),
                        "missing_from_original": sorted(expected_tables - all_lod_names)},
        "map_and_delta_entries": {"retained_count": len(expected_maps),
                                  "original_count": len(actual_games),
                                  "missing_from_original": sorted(expected_maps - actual_games),
                                  "extra_in_original": sorted(actual_games - expected_maps)},
        "archive_entry_counts": {Path(row["relative_path"]).stem: row["inventory"]["entry_count"]
                                 for row in lod_rows},
    }
    if families["text_tables"]["missing_from_original"]:
        problems.append("retained text table absent from original LOD inventory")
    if (families["map_and_delta_entries"]["missing_from_original"]
            or families["map_and_delta_entries"]["extra_in_original"]):
        problems.append("retained map/delta names differ from original Games LOD")
    return {"format": 1, "scope": "F3 bounded original-file snapshot, not F3 semantic acceptance",
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "f1_ledger_sha256": ledger_hashes,
            "retained_catalog_sha256": digest((reference / "catalog.json").read_bytes()),
            "retained_map_probe_sha256": digest((reference / "map-probe.json").read_bytes()),
            "retained_music_manifest_sha256": digest((reference / "music-manifest.json").read_bytes()),
            "status": "pass" if not problems else "needs_reconciliation",
            "problems": problems, "family_reconciliation": families,
            "files": results, "soundtrack": tracks,
            "soundtrack_name_reconciliation": {"missing_from_original": missing_tracks,
                                               "extra_in_original": extra_tracks}}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_app_root", type=Path)
    parser.add_argument("reference", type=Path)
    parser.add_argument("identity_report", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    reference = args.reference.resolve(strict=True)
    output = args.report.resolve()
    if reference == allowed or not reference.is_relative_to(allowed):
        parser.error("reference must stay in ignored local-game-data/mm6")
    if not output.is_relative_to(reference) or output.suffix.lower() != ".json" or not output.parent.is_dir():
        parser.error("report must be JSON inside ignored reference directory")
    if output.exists() or output.is_symlink():
        parser.error("capture report already exists; preserve the first F3 snapshot")
    if subprocess.run(["git", "check-ignore", "-q", str(output)], cwd=repo).returncode != 0:
        parser.error("capture report path must be Git ignored")
    try:
        gate_check(repo, reference, args.identity_report.resolve())
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        parser.error(str(exc))
    original = args.original_app_root.resolve(strict=True)
    if not original.is_dir() or original.is_relative_to(reference):
        parser.error("expected a separate original MM6 app directory")
    try:
        report = capture(original, reference)
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        parser.error(str(exc))
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "input_files": len(report["files"]),
                      "soundtrack_files": len(report["soundtrack"]),
                      "problems": len(report["problems"])}))
    if report["problems"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
