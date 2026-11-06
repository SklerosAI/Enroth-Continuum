#!/usr/bin/env python3
"""F3-only read-only identity check for the owner's extracted MM6 installation.

Prepare this command during F2, but run it only within the consolidated F3
preflight after the integration owner records an F2 freeze. The command hashes
the eight original archives/media/executable named by the retained F1 hash
ledgers and writes a detailed report under ignored local-game-data/mm6.
It does not by itself complete the F3 content/schema reconciliation.
"""

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path, PurePosixPath


LEDGERS = ("archive-sha256.txt", "media-executable-sha256.txt")
EXPECTED_COUNT = 8


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def expected_files(reference: Path) -> tuple[list[dict], dict[str, str]]:
    entries = []
    ledger_hashes = {}
    seen = set()
    for name in LEDGERS:
        ledger = reference / name
        if not ledger.is_file():
            raise ValueError(f"missing retained F1 identity ledger: {name}")
        ledger_hashes[name] = sha256(ledger)
        for line_number, line in enumerate(ledger.read_text(encoding="utf-8").splitlines(), 1):
            match = re.fullmatch(r"([0-9a-fA-F]{64})  (.+)", line)
            if not match:
                raise ValueError(f"bad F1 hash ledger line: {name}:{line_number}")
            expected, old_path = match.groups()
            marker = "/app/"
            if marker not in old_path:
                raise ValueError(f"F1 path lacks app root: {name}:{line_number}")
            relative_text = old_path.rsplit(marker, 1)[1]
            relative = PurePosixPath(relative_text)
            if relative.is_absolute() or ".." in relative.parts or not relative.parts:
                raise ValueError(f"unsafe F1 relative path: {name}:{line_number}")
            key = relative.as_posix().casefold()
            if key in seen:
                raise ValueError(f"duplicate F1 identity path: {name}:{line_number}")
            seen.add(key)
            entries.append({"relative_path": relative.as_posix(), "expected_sha256": expected.lower(),
                            "source_ledger": name})
    if len(entries) != EXPECTED_COUNT:
        raise ValueError(f"expected {EXPECTED_COUNT} F1 identity entries, found {len(entries)}")
    return entries, ledger_hashes


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_app_root", type=Path,
                        help="owner's extracted GOG MM6 app directory; F3 only")
    parser.add_argument("reference", type=Path, help="ignored retained MM6 research directory")
    parser.add_argument("report", type=Path, help="private JSON output inside reference")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    gate = repo / "project/MM6_F2_GATE_REVIEW.md"
    if not any(line.startswith("Status: **frozen") for line in gate.read_text().splitlines()[:10]):
        parser.error("F2 gate is not frozen; original-file preflight is unavailable")
    manifest_path = repo / "local-game-data/mm6/research-2026-11-05/F2_FREEZE_MANIFEST.json"
    if not manifest_path.is_file():
        parser.error("missing ignored F2 freeze manifest")
    manifest = json.loads(manifest_path.read_text())
    current_commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip()
    if manifest.get("source_commit") != current_commit:
        parser.error("source commit differs from the frozen F2 manifest")
    if subprocess.check_output(["git", "status", "--porcelain", "--untracked-files=all"],
                               cwd=repo, text=True).strip():
        parser.error("tracked worktree must be clean before original-file preflight")
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    reference = args.reference.resolve(strict=True)
    output = args.report.resolve()
    if reference == allowed or not reference.is_relative_to(allowed):
        parser.error("reference must stay inside ignored local-game-data/mm6")
    if not output.is_relative_to(reference) or output.suffix.lower() != ".json" or not output.parent.is_dir():
        parser.error("report must be JSON inside the ignored reference directory")
    original = args.original_app_root.resolve(strict=True)
    if not original.is_dir() or original == reference or original.is_relative_to(reference):
        parser.error("original_app_root must be a separate extracted app directory")

    try:
        entries, ledger_hashes = expected_files(reference)
    except ValueError as exc:
        parser.error(str(exc))
    results = []
    for entry in entries:
        path = original.joinpath(*PurePosixPath(entry["relative_path"]).parts)
        if not path.is_file() or path.is_symlink():
            results.append({**entry, "status": "missing_or_symlink"})
            continue
        actual = sha256(path)
        results.append({**entry, "size_bytes": path.stat().st_size, "actual_sha256": actual,
                        "status": "match" if actual == entry["expected_sha256"] else "mismatch"})

    statuses = {status: sum(row["status"] == status for row in results)
                for status in ("match", "mismatch", "missing_or_symlink")}
    report = {"format": 1, "scope": "F3 original identity only; not full content/schema acceptance",
              "f1_ledger_sha256": ledger_hashes, "summary": statuses, "files": results}
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(statuses))
    if statuses["mismatch"] or statuses["missing_or_symlink"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
