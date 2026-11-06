#!/usr/bin/env python3
"""Record a private, content-free fingerprint of the frozen MM6 F2 inputs.

This preparation step never opens the original MM6 installation. It records
hashes of the tracked F2 specification and ignored derived research reports so
the single F3 preflight can detect stale evidence before reading originals.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys


REPO = Path(__file__).resolve().parent.parent
GATE = REPO / "project/MM6_F2_GATE_REVIEW.md"
EVIDENCE = REPO / "local-game-data/mm6/research-2026-11-05"
OUTPUT = EVIDENCE / "F2_FREEZE_MANIFEST.json"


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], cwd=REPO, text=True).strip()


def digest(path: Path) -> str:
    sha = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            sha.update(chunk)
    return sha.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence-root", type=Path, default=EVIDENCE)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args()

    evidence = args.evidence_root.resolve()
    output = args.output.resolve()
    if not GATE.read_text().splitlines()[2].startswith("Status: **frozen"):
        parser.error("F2 gate is not frozen in project/MM6_F2_GATE_REVIEW.md")
    if git("status", "--porcelain", "--untracked-files=all"):
        parser.error("tracked worktree must be clean before freezing F2")
    if not evidence.is_dir():
        parser.error(f"missing derived-evidence root: {evidence}")
    if output.exists():
        parser.error(f"freeze manifest already exists: {output}")
    if subprocess.run(["git", "check-ignore", "-q", str(output)], cwd=REPO).returncode != 0:
        parser.error("output must be inside a Git-ignored private path")

    tracked = git("ls-files", "project/MM6_*.md", "project/plan.md", "scripts/mm6_*.py").splitlines()
    if not tracked:
        parser.error("no tracked MM6 F2 source files found")
    tracked_hashes = {name: digest(REPO / name) for name in tracked}
    report_hashes = {}
    for path in sorted(evidence.rglob("*")):
        if path == output or not path.is_file() or path.is_symlink():
            continue
        if path.suffix.lower() in {".json", ".md"}:
            report_hashes[str(path.relative_to(REPO))] = digest(path)
    if not report_hashes:
        parser.error("no ignored derived F2 reports found")

    payload = {
        "schema": 1,
        "phase": "MM6 F2 frozen inputs; original installation not read",
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "source_commit": git("rev-parse", "HEAD"),
        "source_branch": git("branch", "--show-current"),
        "tracked_sha256": tracked_hashes,
        "ignored_derived_sha256": report_hashes,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + ".tmp")
    temporary.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")
    temporary.replace(output)
    print(f"Frozen F2 manifest: {output}")
    print(f"source={payload['source_commit']} tracked={len(tracked_hashes)} derived={len(report_hashes)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
