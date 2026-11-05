#!/usr/bin/env python3
"""Probe private MM6 map references without launching a game.

Reads only copies previously extracted from Games.lod. The report, including
entry hashes and names, must stay in an ignored local-game-data directory.
"""

import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path


PAIRS = {".odm": ".ddm", ".blv": ".dlv"}


def probe(path: Path) -> dict:
    data = path.read_bytes()
    result = {"name": path.name, "stored_bytes": len(data),
              "sha256": hashlib.sha256(data).hexdigest()}
    if len(data) < 14:
        result["error"] = "too short for MM6 size header and zlib stream"
        return result
    compressed, expected = struct.unpack_from("<II", data)
    result.update(compressed_bytes=compressed, expected_bytes=expected,
                  header_matches=compressed == len(data) - 8)
    if not result["header_matches"]:
        result["error"] = "compressed size header differs from entry size"
        return result
    payload = data[8:]
    try:
        decoded = zlib.decompress(payload)
        result["checksum_valid"] = True
    except zlib.error as exc:
        result["checksum_valid"] = False
        result["zlib_error"] = str(exc)
        try:
            inflater = zlib.decompressobj(-15)
            decoded = inflater.decompress(payload[2:-4]) + inflater.flush()
        except zlib.error as raw_exc:
            result["error"] = f"raw DEFLATE also failed: {raw_exc}"
            return result
        if not inflater.eof or inflater.unconsumed_tail:
            result["error"] = "raw DEFLATE stream did not reach its end"
            return result
        result["trailing_payload_bytes"] = len(inflater.unused_data)
        result["stored_adler32"] = payload[-4:].hex()
        result["actual_adler32"] = f"{zlib.adler32(decoded):08x}"
    result["decoded_bytes"] = len(decoded)
    result["decoded_size_matches"] = len(decoded) == expected
    result["decoded_sha256"] = hashlib.sha256(decoded).hexdigest()
    if not result["decoded_size_matches"]:
        result["error"] = "decoded size differs from header"
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="ignored output JSON within reference")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents:
        parser.error("report must remain in the private reference directory")
    games = root / "Games"
    if not games.is_dir():
        parser.error("Games extraction is missing")

    result = {"format": 1, "maps": [], "missing_pairs": [], "extra_pairs": []}
    bases = set()
    for suffix, delta_suffix in PAIRS.items():
        for path in sorted(games.glob(f"*{suffix}")):
            bases.add(path.stem.lower())
            result["maps"].append(probe(path))
            delta = path.with_suffix(delta_suffix)
            if delta.is_file():
                result["maps"].append(probe(delta))
            else:
                result["missing_pairs"].append(path.name)
    for suffix in PAIRS.values():
        for path in games.glob(f"*{suffix}"):
            if path.stem.lower() not in bases:
                result["extra_pairs"].append(path.name)

    errors = [entry for entry in result["maps"] if "error" in entry]
    checksum_failures = [entry["name"] for entry in result["maps"] if not entry.get("checksum_valid", False)]
    result["summary"] = {"entries": len(result["maps"]), "fatal_errors": len(errors),
                         "checksum_failures": checksum_failures}
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Probed {len(result['maps'])} map entries; "
          f"{len(errors)} fatal errors; {len(checksum_failures)} checksum failures")
    for entry in errors:
        print(f"  ERROR {entry['name']}: {entry['error']}")
    for name in checksum_failures:
        entry = next(item for item in result["maps"] if item["name"] == name)
        print(f"  FRAMING {name}: decoded size matched; "
              f"{entry.get('trailing_payload_bytes', 0)} bytes follow DEFLATE before the final trailer")
    if errors or result["missing_pairs"] or result["extra_pairs"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
