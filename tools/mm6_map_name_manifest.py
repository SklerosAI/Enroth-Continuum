#!/usr/bin/env python3
"""Build/verify a private MM6 .ec6 MAPS name manifest from retained F3 evidence.

Only aggregate results are printed. The output stays under ignored local-game-data.
This never reads the original game installation or decodes a game archive.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re


GATE = "F3_GATE_REPORT_HASHES_2026-11-06.json"
CAPTURE = "F3_CAPTURE_2026-11-05.json"
REPORT = "F3_CONTENT_RECONCILIATION_2026-11-05.json"
SIGNOFF = "F3_CONTENT_SCHEMA_SIGNOFF_2026-11-05.json"
PROBE = "map-probe.json"
MANIFEST = "MM6_EC6_MAP_NAME_MANIFEST_F4.json"
NAME = re.compile(r"[a-z0-9_-]{1,12}\.(?:ddm|dlv)\Z")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_json(path):
    return json.loads(path.read_bytes())


def require(ok, message):
    if not ok:
        raise ValueError(message)


def derive(root):
    gate = read_json(root / GATE)
    capture = read_json(root / CAPTURE)
    report = read_json(root / REPORT)
    signoff = read_json(root / SIGNOFF)
    probe = read_json(root / PROBE)

    for filename in (CAPTURE, REPORT, SIGNOFF):
        require(digest((root / filename).read_bytes()) == gate["reports_sha256"][filename],
                f"F3 gate hash mismatch: {filename}")
    require(report["capture_sha256"] == digest((root / CAPTURE).read_bytes()),
            "F3 capture link mismatch")
    require(signoff["f3_content_report_sha256"] == digest((root / REPORT).read_bytes()),
            "F3 signoff link mismatch")
    require(capture["retained_map_probe_sha256"] == digest((root / PROBE).read_bytes()),
            "F3 map-probe link mismatch")
    require(capture["status"] == report["status"] == signoff["status"] == "pass",
            "F3 chain is not signed off")
    require(report["probe_checks"][PROBE]["rows"] == 134 and
            report["probe_checks"][PROBE]["sha256_matches"] == 134 and
            not report["probe_checks"][PROBE]["mismatches"],
            "F3 map probe reconciliation is incomplete")
    require(report["families"]["map_pairs"]["count"] == 134 and
            report["families"]["map_pairs"]["status_counts"] == {"match": 134},
            "F3 map pair reconciliation is incomplete")
    check = signoff["checks"]["maps"]
    require(check["count"] == check["matches"] == 134 and
            len(check["rows"]) == 134 and all(row["match"] for row in check["rows"]),
            "F3 map signoff is incomplete")
    require(probe["summary"]["entries"] == len(probe["maps"]) == 134 and
            not probe["missing_pairs"] and not probe["extra_pairs"],
            "F3 map probe has missing or extra pairs")

    signed = {row["name"].lower(): row["source_sha256"] for row in check["rows"]}
    require(len(signed) == 134, "F3 signoff names are not unique")
    rows = {}
    for row in probe["maps"]:
        name = row["name"].lower()
        require(name not in rows and signed.get(name) == row["sha256"],
                "F3 map name or source hash differs from signoff")
        rows[name] = row
    require(set(rows) == set(signed), "F3 probe and signoff names differ")

    entries = []
    for suffix, directory, base_suffix, count in (
        (".dlv", "F4_INDOOR_DECODED", ".blv", 52),
        (".ddm", "F4_OUTDOOR_DECODED", ".odm", 15),
    ):
        selected = sorted(name for name in rows if name.endswith(suffix))
        require(len(selected) == count, "F3 delta-family count differs")
        directory_path = root / directory
        actual = {p.name.lower() for p in directory_path.iterdir() if p.is_file() and p.suffix.lower() == suffix}
        require(actual == set(selected), "retained decoded delta name set differs")
        for name in selected:
            require(NAME.fullmatch(name) is not None, "invalid F3 map name")
            require(name[:-4] + base_suffix in rows, "F3 delta has no base map")
            decoded_hash = digest((directory_path / name).read_bytes())
            require(decoded_hash == rows[name]["decoded_sha256"],
                    "retained decoded delta hash differs from F3")
            entries.append({"name": name, "decoded_sha256": decoded_hash})

    entries.sort(key=lambda entry: entry["name"])
    names = [entry["name"] for entry in entries]
    require(len(entries) == len(set(names)) == 67, "expected exactly 67 unique delta names")
    return {
        "format": "mm6-ec6-map-name-manifest-v1",
        "source_gate_sha256": digest((root / GATE).read_bytes()),
        "source_probe_sha256": digest((root / PROBE).read_bytes()),
        "names_sha256": digest(("\n".join(names) + "\n").encode("ascii")),
        "entries": entries,
    }


def validate_candidate(manifest, candidate_path):
    candidate = read_json(candidate_path)
    require(isinstance(candidate, list) and len(candidate) == 67 and
            all(isinstance(name, str) and NAME.fullmatch(name) for name in candidate) and
            len(set(candidate)) == 67, "candidate must have 67 unique valid names")
    require(set(candidate) == {entry["name"] for entry in manifest["entries"]},
            "candidate map names differ from F3 manifest")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--research", type=Path, required=True,
                        help="ignored retained F3 research directory")
    parser.add_argument("--check-names", type=Path,
                        help="optional JSON array of 67 candidate delta names")
    args = parser.parse_args()
    root = args.research.resolve(strict=True)
    require("local-game-data" in root.parts, "research must be under ignored local-game-data")
    expected = derive(root)
    output = root / MANIFEST
    if output.exists():
        require(read_json(output) == expected, "private manifest differs from F3 evidence")
    else:
        descriptor = os.open(output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(descriptor, "w") as stream:
            stream.write(json.dumps(expected, indent=2) + "\n")
    output.chmod(0o600)
    if args.check_names:
        validate_candidate(expected, args.check_names)
    print("MM6 private MAPS manifest verified: 67 names, 67 decoded hashes, F3 chain intact")


if __name__ == "__main__":
    main()
