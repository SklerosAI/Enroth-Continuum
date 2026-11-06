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
MUSIC_COUNT = 15


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


def expected_music_files(reference: Path) -> tuple[list[dict], str]:
    manifest_path = reference / "music-manifest.json"
    if not manifest_path.is_file() or manifest_path.is_symlink():
        raise ValueError("missing retained music manifest")
    rows = json.loads(manifest_path.read_text(encoding="utf-8"))
    if not isinstance(rows, list) or len(rows) != MUSIC_COUNT:
        raise ValueError(f"expected {MUSIC_COUNT} retained music entries")
    entries = []
    seen = set()
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError("invalid retained music entry")
        name, size, expected = row.get("file"), row.get("bytes"), row.get("sha256")
        if (not isinstance(name, str) or not re.fullmatch(r"[A-Za-z0-9_-]+\.mp3", name)
                or isinstance(size, bool) or not isinstance(size, int) or size <= 0
                or not isinstance(expected, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", expected)):
            raise ValueError("invalid retained music name, size or hash")
        if name.casefold() in seen:
            raise ValueError("duplicate retained music name")
        seen.add(name.casefold())
        entries.append({"relative_path": f"Sounds/{name}", "expected_sha256": expected.lower(),
                        "expected_size_bytes": size, "source_ledger": "music-manifest.json"})
    return entries, sha256(manifest_path)


def verify_frozen_inputs(repo: Path, reference: Path, manifest: dict) -> None:
    """Fail before original-file access if any F2 source/report fingerprint drifts."""
    repo = repo.resolve()
    reference = reference.resolve()
    if not isinstance(manifest, dict) or manifest.get("schema") != 1:
        raise ValueError("invalid F2 freeze manifest schema")
    derived = manifest.get("ignored_derived_sha256")
    required = {str((reference / name).relative_to(repo))
                for name in (*LEDGERS, "music-manifest.json")}
    if not isinstance(derived, dict) or not required.issubset(derived):
        raise ValueError("F2 freeze manifest lacks original identity inputs")
    for label, entries in (("tracked", manifest.get("tracked_sha256")),
                           ("ignored derived", manifest.get("ignored_derived_sha256"))):
        if not isinstance(entries, dict) or not entries:
            raise ValueError(f"F2 freeze manifest lacks {label} hashes")
        for relative, expected in entries.items():
            if (not isinstance(relative, str) or not isinstance(expected, str)
                    or not re.fullmatch(r"[0-9a-f]{64}", expected)):
                raise ValueError(f"invalid {label} manifest entry")
            relative_path = PurePosixPath(relative)
            if not relative_path.parts or relative_path.is_absolute() or ".." in relative_path.parts:
                raise ValueError(f"unsafe {label} manifest path")
            path = repo.joinpath(*relative_path.parts)
            resolved = path.resolve()
            if not resolved.is_relative_to(repo):
                raise ValueError(f"{label} manifest path escapes repository")
            if label == "ignored derived" and not resolved.is_relative_to(reference):
                raise ValueError("derived report path escapes frozen reference")
            if not path.is_file() or path.is_symlink():
                raise ValueError(f"missing or symlinked frozen {label} input: {relative}")
            if sha256(path) != expected:
                raise ValueError(f"changed frozen {label} input: {relative}")


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
    try:
        manifest = json.loads(manifest_path.read_text())
    except (OSError, json.JSONDecodeError) as exc:
        parser.error(f"invalid F2 freeze manifest: {exc}")
    if not isinstance(manifest, dict):
        parser.error("invalid F2 freeze manifest structure")
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
    if output.exists():
        parser.error("F3 identity report already exists; preserve the first comparison")
    if subprocess.run(["git", "check-ignore", "-q", str(output)], cwd=repo).returncode != 0:
        parser.error("F3 identity report path must be Git ignored")
    try:
        verify_frozen_inputs(repo, reference, manifest)
    except ValueError as exc:
        parser.error(str(exc))
    original = args.original_app_root.resolve(strict=True)
    if not original.is_dir() or original == reference or original.is_relative_to(reference):
        parser.error("original_app_root must be a separate extracted app directory")

    try:
        entries, ledger_hashes = expected_files(reference)
        music_entries, music_hash = expected_music_files(reference)
        entries.extend(music_entries)
        ledger_hashes["music-manifest.json"] = music_hash
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        parser.error(str(exc))
    results = []
    for entry in entries:
        path = original.joinpath(*PurePosixPath(entry["relative_path"]).parts)
        if not path.resolve().is_relative_to(original) or not path.is_file() or path.is_symlink():
            results.append({**entry, "status": "missing_or_symlink"})
            continue
        actual = sha256(path)
        size = path.stat().st_size
        size_match = size == entry.get("expected_size_bytes", size)
        results.append({**entry, "size_bytes": path.stat().st_size, "actual_sha256": actual,
                        "status": "match" if actual == entry["expected_sha256"] and size_match else "mismatch"})

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
