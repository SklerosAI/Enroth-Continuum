#!/usr/bin/env python3
"""Run every MM7 fresh-process checkpoint pair before creating a release tag.

This is an explicit, local-only gate. It never creates a tag or reads owner
saves; each writer and reader uses a separate fresh game process in a marked
temporary profile. Ordinary builds, default GameTests and CI do not call it.
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


REPO = Path(__file__).resolve().parents[1]
REGISTRY = REPO / "test/Bin/GameTest/GameTestMain.cpp"
MAX_PAIRS = 48
MIN_FREE_BYTES = 10 * 1024**3
TEST_TIMEOUT_SECONDS = 30 * 60
MARKER = ".ec-mm7-disposable-save-gate"
MARKER_TEXT = "EnrothContinuum-MM7-save-gate-v1\n"


def checkpoint_pairs() -> list[tuple[str, str]]:
    source = REGISTRY.read_text()
    match = re.search(
        r"constexpr std::array<Mm7ProfileGate, (\d+)> mm7ProfileGates = \{\{(.*?)\}\};",
        source, re.DOTALL,
    )
    if not match:
        raise RuntimeError("MM7 profile-gate registry was not found")
    entries = re.findall(
        r'\{"([^"]+)",\s*(true|false),\s*(true|false),\s*(-?\d+)\}', match.group(2),
    )
    if len(entries) != int(match.group(1)):
        raise RuntimeError("MM7 profile-gate registry count does not match its declaration")
    checkpoints = [entry for entry in entries if entry[3] == "-1" and
                   not entry[0].startswith("MM7OwnedSaveProbe.")]
    if len(checkpoints) % 2:
        raise RuntimeError("MM7 checkpoint registry has an unpaired test")
    pairs = []
    for writer, reader in zip(checkpoints[::2], checkpoints[1::2]):
        if (writer[1:] != ("true", "false", "-1") or
                reader[1:] != ("false", "true", "-1") or
                writer[0].split(".")[0] != reader[0].split(".")[0] or
                ".DISABLED_" not in writer[0] or ".DISABLED_" not in reader[0]):
            raise RuntimeError(f"invalid MM7 checkpoint pair near {writer[0]}")
        pairs.append((writer[0], reader[0]))
    if not pairs or len(pairs) > MAX_PAIRS:
        raise RuntimeError(f"MM7 checkpoint count must be between 1 and {MAX_PAIRS}")
    return pairs


def require_release_candidate(tag: str) -> str:
    if not re.fullmatch(r"ECv[0-9]+\.[0-9]+\.[0-9]+", tag):
        raise ValueError("candidate tag must look like ECv0.2.0")
    existing = subprocess.run(
        ["git", "show-ref", "--verify", "--quiet", f"refs/tags/{tag}"], cwd=REPO,
        check=False,
    )
    if existing.returncode == 0:
        raise ValueError("candidate tag already exists; this gate must run before tagging")
    if existing.returncode != 1:
        raise RuntimeError("could not check candidate tag")
    status = subprocess.check_output(["git", "status", "--porcelain"], cwd=REPO)
    if status:
        raise ValueError("commit or discard working-tree changes before the pre-tag gate")
    return subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO, text=True).strip()


def run_pair(binary: Path, data: Path, test_data: Path, pair: tuple[str, str]) -> None:
    with tempfile.TemporaryDirectory(prefix="mm7-save-gate-release-") as temporary:
        profile = Path(temporary)
        (profile / MARKER).write_text(MARKER_TEXT)
        for test in pair:
            command = (
                str(binary), "--game=mm7", "--headless", f"--data-path={data}",
                f"--test-path={test_data}", f"--mm7-disposable-profile={profile}",
                f"--gtest_filter={test}", "--gtest_also_run_disabled_tests",
                "--log-level=critical",
            )
            print(f"Running {test}", flush=True)
            subprocess.run(command, check=True, timeout=TEST_TIMEOUT_SECONDS, cwd=REPO)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="list pairs without running the game")
    parser.add_argument("--candidate-tag", help="new ECvX.Y.Z tag intended after this gate passes")
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--data-path", type=Path)
    parser.add_argument("--test-path", type=Path)
    args = parser.parse_args()
    pairs = checkpoint_pairs()
    if args.list:
        for index, (writer, reader) in enumerate(pairs, 1):
            print(f"{index:02}/{len(pairs)} {writer} -> {reader}")
        return
    if not args.candidate_tag or not args.binary or not args.data_path or not args.test_path:
        parser.error("release run requires --candidate-tag, --binary, --data-path and --test-path")
    for path in (args.binary, args.data_path, args.test_path):
        if not path.exists():
            parser.error(f"missing path: {path}")
    revision = require_release_candidate(args.candidate_tag)
    binary = args.binary.resolve()
    data = args.data_path.resolve()
    test_data = args.test_path.resolve()
    print(f"Pre-tag MM7 checkpoint gate: {len(pairs)} pairs at {revision}", flush=True)
    for index, pair in enumerate(pairs, 1):
        if shutil.disk_usage(REPO).free < MIN_FREE_BYTES:
            raise RuntimeError("free disk space is below the 10 GiB floor")
        print(f"Pair {index}/{len(pairs)}", flush=True)
        run_pair(binary, data, test_data, pair)
    print(f"All {len(pairs)} MM7 checkpoint pairs passed for {args.candidate_tag}; tag was not created")


if __name__ == "__main__":
    main()
