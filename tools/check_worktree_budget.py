#!/usr/bin/env python3
"""Refuse a new worktree when the Enroth checkout budget is exhausted."""

import pathlib
import subprocess
import sys


LIMIT = 15
ROOT = pathlib.Path(__file__).resolve().parents[1]


def main() -> int:
    result = subprocess.run(
        ["git", "-C", str(ROOT), "worktree", "list", "--porcelain"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode:
        print(result.stderr.strip() or "Could not list Git worktrees.", file=sys.stderr)
        return 2
    count = sum(line.startswith("worktree ") for line in result.stdout.splitlines())
    print(f"Enroth worktrees: {count}/{LIMIT}")
    if count >= LIMIT:
        print("Reuse or clean up a finished worktree before creating another.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
