#!/usr/bin/env python3
"""Run the MM7 input-driven movement, combat, and optional Barrow harnesses."""

import argparse
from pathlib import Path
import subprocess
import sys


REPO = Path(__file__).resolve().parents[1]
LIST_TIMEOUT_SECONDS = 5 * 60
SUITE_TIMEOUT_SECONDS = 30 * 60
CORE_SUITES = ("MM7GameplayHarness", "MM7CombatHarness")
BARROW_SUITE = "MM7BarrowTraversal"


def list_tests(game_test):
    command = [
        str(game_test),
        "--game", "mm7",
        "--headless",
        "--gtest_list_tests",
        "--gtest_filter=" + ":".join(f"{suite}.*" for suite in (*CORE_SUITES, BARROW_SUITE)),
    ]
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            check=False, timeout=LIST_TIMEOUT_SECONDS)
    if result.returncode:
        print(result.stdout, file=sys.stderr)
        raise RuntimeError(f"listing MM7 gameplay harness tests failed with exit {result.returncode}")

    listed = set()
    suite = ""
    for line in result.stdout.splitlines():
        if line and not line.startswith(" "):
            suite = line.rstrip(".").strip()
        elif line.strip():
            listed.add(f"{suite}.{line.split('#')[0].strip()}")
    return listed


def require_suites(listed, suites):
    for suite in suites:
        if not any(test.startswith(f"{suite}.") for test in listed):
            raise RuntimeError(f"required suite {suite} has no tests in the GameTest binary")


def run_suite(args, suite):
    command = [
        str(args.game_test),
        "--game", "mm7",
        "--headless",
        "--data-path", str(args.data_path),
        "--test-path", str(args.test_path),
        f"--gtest_filter={suite}.*",
        "--gtest_brief=1",
        "--log-level=critical",
    ]
    try:
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                check=False, timeout=SUITE_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired as error:
        print(error.stdout or "", file=sys.stderr)
        raise RuntimeError(f"{suite} timed out after {SUITE_TIMEOUT_SECONDS} seconds") from error
    print(result.stdout, end="")
    if result.returncode:
        raise RuntimeError(f"{suite} failed with exit {result.returncode}")
    print(f"{suite}: passed in a fresh headless process")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-path", type=Path, help="Local MM7 game-data directory; required unless --list")
    parser.add_argument("--game-test", type=Path, default=REPO / "build/test/Bin/GameTest/OpenEnroth_GameTest")
    parser.add_argument("--test-path", type=Path, default=REPO / "test/Data")
    parser.add_argument("--with-barrows", action="store_true", help="Also require and run the Barrow traversal suite")
    parser.add_argument("--list", action="store_true", help="List admitted harness tests without running them")
    return parser.parse_args()


def main(args):
    if not args.game_test.is_file() or not args.test_path.is_dir():
        raise ValueError("GameTest binary or test-data directory is missing")
    if not args.list and (args.data_path is None or not args.data_path.is_dir()):
        raise ValueError("MM7 data path is missing")

    suites = CORE_SUITES + ((BARROW_SUITE,) if args.with_barrows else ())
    listed = list_tests(args.game_test)
    require_suites(listed, suites)
    if args.list:
        for test in sorted(listed):
            print(test)
        return 0
    for suite in suites:
        run_suite(args, suite)
    print("MM7 gameplay harness: all required suites passed")
    return 0


if __name__ == "__main__":
    arguments = parse_args()
    try:
        sys.exit(main(arguments))
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"MM7 gameplay harness: {error}", file=sys.stderr)
        sys.exit(1)
