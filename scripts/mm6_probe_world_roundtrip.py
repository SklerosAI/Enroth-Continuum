#!/usr/bin/env python3
"""Round-trip retained MM6 map deltas by bounded wire sections, offline.

This checks byte preservation and structural rejection only. It does not infer
record field semantics, implement the C++ runtime, or prove save compatibility.
Detailed map names and hashes are written only to an ignored local report.
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path

from mm6_probe_indoors import FormatError, decode, probe as probe_indoor_base
from mm6_probe_outdoors import probe_base as probe_outdoor_base


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def split_delta(data: bytes, indoor: bool, door_bytes: int = 0) -> list[tuple[str, bytes]]:
    """Parse section spans while preserving every wire byte in its source order."""
    result: list[tuple[str, bytes]] = []
    offset = 0

    def take(name: str, size: int) -> None:
        nonlocal offset
        if size < 0 or size > len(data) - offset:
            raise FormatError(f"{name} overruns decoded delta")
        result.append((name, data[offset:offset + size]))
        offset += size

    def counted(name: str, width: int) -> None:
        if len(data) - offset < 4:
            raise FormatError(f"missing {name} count")
        count = struct.unpack_from("<I", data, offset)[0]
        if count > (len(data) - offset - 4) // width:
            raise FormatError(f"{name} count overruns decoded delta")
        take(name, 4 + count * width)

    take("reset_header", 8)
    take("visibility", 875 if indoor else 2 * 88 * 11)
    counted("creatures", 548)
    counted("ground_items", 100)
    counted("chests", 4204)
    if indoor:
        if door_bytes < 0:
            raise FormatError("negative door variable-array size")
        door_start = offset
        take("door_records", 200 * 80)
        computed = 0
        for slot in range(200):
            vertices, faces, rooms, offsets = struct.unpack_from("<4h", data, door_start + slot * 80 + 68)
            if min(vertices, faces, rooms, offsets) < 0:
                raise FormatError("negative door variable-array count")
            computed += 2 * (vertices + 3 * faces + rooms + 3 * offsets)
        if computed != door_bytes:
            raise FormatError("door variable-array size differs from base-map header")
        take("door_variable_arrays", door_bytes)
    take("persistent_bytes", 200)
    take("tail", 56)
    if offset != len(data):
        raise FormatError("unclaimed bytes after delta tail")
    return result


def reassemble(parts: list[tuple[str, bytes]]) -> bytes:
    return b"".join(chunk for _, chunk in parts)


def check_mutations(parts: list[tuple[str, bytes]], indoor: bool, door_bytes: int) -> int:
    """Inject nonzero sentinels without changing count fields or section widths."""
    checked = 0
    original = dict(parts)
    for name in ("reset_header", "visibility", "persistent_bytes", "tail"):
        index = next(i for i, (label, _) in enumerate(parts) if label == name)
        raw = bytearray(parts[index][1])
        for position in {0, len(raw) // 2, len(raw) - 1}:
            raw[position] ^= 0xA5
        changed = list(parts)
        changed[index] = (name, bytes(raw))
        roundtrip = split_delta(reassemble(changed), indoor, door_bytes)
        if dict(roundtrip)[name] != bytes(raw):
            raise FormatError(f"sentinel lost in {name}")
        if any(chunk != original[label] for label, chunk in roundtrip if label != name):
            raise FormatError(f"sentinel in {name} changed another section")
        checked += 1
    return checked


def check_invalid_inputs(data: bytes, indoor: bool, door_bytes: int) -> int:
    checks = 0
    for length in (len(data) - 1, 8, 0):
        try:
            split_delta(data[:length], indoor, door_bytes)
        except FormatError:
            checks += 1
        else:
            raise FormatError("truncated delta was accepted")
    count_offset = 8 + (875 if indoor else 2 * 88 * 11)
    overflow = bytearray(data)
    struct.pack_into("<I", overflow, count_offset, 0xFFFFFFFF)
    try:
        split_delta(bytes(overflow), indoor, door_bytes)
    except FormatError:
        checks += 1
    else:
        raise FormatError("overflowing creature count was accepted")
    return checks


def check_one(path: Path, indoor: bool) -> dict:
    base_path = path.with_suffix(".blv" if indoor else ".odm")
    base = probe_indoor_base(base_path) if indoor else probe_outdoor_base(base_path)
    if "error" in base:
        raise FormatError("base map layout did not pass the existing bounded probe")
    if indoor and base["doors"] != 200:
        raise FormatError("indoor base map does not declare 200 door slots")
    door_bytes = base["variable_bytes"]["doors"] if indoor else 0
    data, checksum_valid, trailing_bytes = decode(path)
    if not checksum_valid or trailing_bytes:
        raise FormatError("delta checksum or framing failed")
    parts = split_delta(data, indoor, door_bytes)
    if reassemble(parts) != data:
        raise FormatError("section reassembly changed decoded bytes")
    mutations = check_mutations(parts, indoor, door_bytes)
    rejected = check_invalid_inputs(data, indoor, door_bytes)
    fixed = {name: chunk for name, chunk in parts if name in ("reset_header", "persistent_bytes", "tail")}
    return {
        "name": path.name,
        "entry_sha256": sha256(path.read_bytes()),
        "decoded_sha256": sha256(data),
        "decoded_size": len(data),
        "sections": [{"name": name, "size": len(chunk), "sha256": sha256(chunk)} for name, chunk in parts],
        "fixed_nonzero": {name: sum(byte != 0 for byte in chunk) for name, chunk in fixed.items()},
        "sentinel_regions_checked": mutations,
        "malformed_cases_rejected": rejected,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="retained ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="JSON report under that directory")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if not root.is_relative_to(allowed) or root == allowed:
        parser.error("reference must be an ignored MM6 research directory")
    if not report.is_relative_to(root) or report.suffix.lower() != ".json" or not report.parent.is_dir():
        parser.error("report must be a JSON file inside the ignored reference directory")
    games = root / "Games"
    indoor_paths = sorted(games.glob("*.dlv"))
    outdoor_paths = sorted(games.glob("*.ddm"))
    if len(indoor_paths) != 52 or len(outdoor_paths) != 15:
        parser.error("expected exactly 52 indoor and 15 outdoor retained deltas")
    entries = []
    failures = []
    for path, indoor in [(path, True) for path in indoor_paths] + [(path, False) for path in outdoor_paths]:
        try:
            entries.append(check_one(path, indoor))
        except (FormatError, OSError, struct.error) as exc:
            failures.append({"name": path.name, "error": str(exc)})
    summary = {
        "indoor_deltas": len(indoor_paths),
        "outdoor_deltas": len(outdoor_paths),
        "byte_exact_roundtrips": len(entries),
        "sentinel_region_checks": sum(entry["sentinel_regions_checked"] for entry in entries),
        "malformed_cases_rejected": sum(entry["malformed_cases_rejected"] for entry in entries),
        "deltas_with_nonzero_reset_header": sum(entry["fixed_nonzero"]["reset_header"] != 0 for entry in entries),
        "deltas_with_nonzero_persistent_bytes": sum(entry["fixed_nonzero"]["persistent_bytes"] != 0 for entry in entries),
        "deltas_with_nonzero_tail": sum(entry["fixed_nonzero"]["tail"] != 0 for entry in entries),
        "failures": len(failures),
    }
    result = {
        "format": 1,
        "scope": "retained delta wire sections only; no original archives opened or runtime launched",
        "warning": "Byte-preserving section round-trip is not MM6 field semantics or an engine save implementation.",
        "summary": summary,
        "entries": entries,
        "failures": failures,
        "source_sha256": {name: sha256((repo / name).read_bytes()) for name in (
            "scripts/mm6_probe_world_roundtrip.py", "scripts/mm6_probe_indoors.py",
            "scripts/mm6_probe_outdoors.py", "src/Engine/SaveLoad.cpp",
            "src/Engine/Graphics/Indoor.cpp", "src/Engine/Graphics/Outdoor.cpp")},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
