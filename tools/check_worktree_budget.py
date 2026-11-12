#!/usr/bin/env python3
"""Refuse a new worktree when an agent's share of the checkout budget is used up.

Claude (all MM6 work) and Codex (MM7 work) get five linked worktrees each.
The main checkout is the shared integration tree and counts against neither.
Linked worktrees are assigned by their `claude/` or `codex/` branch prefix;
unassigned legacy worktrees are reported but do not silently consume a title's
allocation.

Usage: check_worktree_budget.py --agent claude|codex
"""

import argparse
import pathlib
import subprocess
import sys


LIMIT_PER_AGENT = 5
ROOT = pathlib.Path(__file__).resolve().parents[1]


def owner(branch: str) -> str | None:
    if branch.startswith("refs/heads/codex/"):
        return "codex"
    if branch.startswith("refs/heads/claude/"):
        return "claude"
    return None


def worktree_records(output: str) -> list[dict[str, str]]:
    records = []
    for block in output.strip().split("\n\n"):
        record = {}
        for line in block.splitlines():
            key, _, value = line.partition(" ")
            record[key] = value
        if "worktree" in record:
            records.append(record)
    return records


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--agent", choices=["claude", "codex"], required=True)
    args = parser.parse_args()

    result = subprocess.run(
        ["git", "-C", str(ROOT), "worktree", "list", "--porcelain"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode:
        print(result.stderr.strip() or "Could not list Git worktrees.", file=sys.stderr)
        return 2
    paths = worktree_records(result.stdout)
    counts = {"claude": 0, "codex": 0}
    unassigned = []
    for record in paths:
        path = pathlib.Path(record["worktree"])
        if record.get("branch") == "refs/heads/master":
            continue
        assigned_owner = owner(record.get("branch", ""))
        if assigned_owner:
            counts[assigned_owner] += 1
        else:
            unassigned.append(path)
    for agent, count in counts.items():
        print(f"{agent} worktrees: {count}/{LIMIT_PER_AGENT}")
    for path in unassigned:
        print(f"unassigned worktree: {path}")
    if counts[args.agent] >= LIMIT_PER_AGENT:
        print("Reuse or clean up a finished worktree before creating another.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
