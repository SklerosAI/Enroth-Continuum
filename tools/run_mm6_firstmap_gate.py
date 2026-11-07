#!/usr/bin/env python3
"""Run the disposable MM6 first-map v5 handoff gate in fresh child processes.

Private data paths are supplied by the caller. Each chain uses an owner-only
temporary profile that is removed when the run ends. This never writes a
production save or changes the installed Atlas.
"""

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
GAME_TEST = ROOT / "build/test/Bin/GameTest/OpenEnroth_GameTest"

SEED = ("MM6ZCreationItemSeedHeadless.DISABLED_WriteRealStarterV2RestartSlot", None, None)
V4 = ("MM6V4PostEntryCandidateHeadless.DISABLED_FirstMapFullSessionPreflight",
      "ENROTH_MM6_V4_WRITE_ENABLE", "strict-v4")
V5_WRITE = ("MM6V5AfterActionWriterHeadless.DISABLED_FreshQuickLoadWritesExactMovedPartyAndSprites",
            "ENROTH_MM6_V5_AFTER_ACTION_WRITE_ENABLE", "strict-v5-after-action")
SEVEN_CHILD_CHAIN = [
    SEED,
    V4,
    V5_WRITE,
    ("MM6V5AfterActionFreshChildReplayHeadless.DISABLED_PostActionQuickLoadRestoresSpritesBeforeInputAndAborts",
     "ENROTH_MM6_V5_AFTER_ACTION_REPLAY_ENABLE", "strict-v5-after-action"),
    ("MM6V5FirstInputHeadless.DISABLED_PostActionQuickLoadMovesOnceInRealLoop",
     "ENROTH_MM6_V5_FIRST_INPUT_ENABLE", "strict-v5-after-action"),
    ("MM6V5SecondWriteHeadless.DISABLED_PostReloadInputWritesDistinctCandidate",
     "ENROTH_MM6_V5_SECOND_WRITE_ENABLE", "strict-v5-second-write"),
    ("MM6V5SecondFreshChildReplayHeadless.DISABLED_SecondQuickLoadRestoresOwnersBeforeInputAndAborts",
     "ENROTH_MM6_V5_SECOND_REPLAY_ENABLE", "strict-v5-second-replay"),
]
FAULT_CHILDREN = [
    ("MM6V5FirstMapAbandonHeadless.DISABLED_PartialAssignmentThenFullReloadAndInput",
     "ENROTH_MM6_V5_ABANDON_ENABLE", "strict-v5-abandon"),
    ("MM6V5FirstMapAbandonHeadless.DISABLED_PreEventFaultThenFullReloadAndInput",
     "ENROTH_MM6_V5_ABANDON_ENABLE", "strict-v5-abandon"),
]
MM7_CONTROL = "Prs.Pr2354"
PASS_LINE = re.compile(r"\[\s*PASSED\s*\]\s+1 test\b")


def run_child(name, flag, value, profile, data, rules=None):
    env = os.environ.copy()
    for key in list(env):
        if key.startswith("ENROTH_MM6_V") or key == "ENROTH_MM6_RESTART_PROFILE":
            del env[key]
    if rules:
        env["ENROTH_MM6_RESTART_PROFILE"] = str(profile)
    if flag:
        env[flag] = value
    command = [str(GAME_TEST), "--game", "mm6" if rules else "mm7",
               "--headless", "--data-path", str(data)]
    if rules:
        command += ["--rules-path", str(rules)]
    command += ["--test-path", "test/Data", "--gtest_filter=" + name,
                "--gtest_brief=1"]
    if rules:
        command += ["--gtest_also_run_disabled_tests"]
    try:
        result = subprocess.run(command, cwd=ROOT, env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, timeout=240, check=False)
    except subprocess.TimeoutExpired:
        print(f"FAIL {name}: timed out", file=sys.stderr)
        return False
    if result.returncode == 0 and PASS_LINE.search(result.stdout):
        print(f"PASS {name}")
        return True
    print(f"FAIL {name}: exit {result.returncode}", file=sys.stderr)
    print("\n".join(result.stdout.splitlines()[-18:]), file=sys.stderr)
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="show the gate without building or using private data")
    parser.add_argument("--skip-build", action="store_true", help="use the existing GameTest binary")
    args = parser.parse_args()
    if args.list:
        for name, _, _ in SEVEN_CHILD_CHAIN:
            print("seven-child:", name)
        for name, _, _ in FAULT_CHILDREN:
            print("fault-retry:", name)
        print("MM7 control:", MM7_CONTROL)
        return 0

    paths = {}
    for key in ("MM6_DATA_PATH", "MM6_RULES_PATH", "MM7_DATA_PATH"):
        value = os.environ.get(key)
        if not value or not Path(value).is_dir():
            print(f"Set {key} to an existing private directory.", file=sys.stderr)
            return 2
        paths[key] = Path(value)
    if not args.skip_build:
        owned_cmake = Path.home() / "enroth-tools/venv/bin/cmake"
        cmake = str(owned_cmake) if owned_cmake.is_file() else shutil.which("cmake")
        if not cmake or cmake.startswith("/opt/homebrew/"):
            print("Provide the project CMake toolchain outside Homebrew.", file=sys.stderr)
            return 2
        result = subprocess.run([cmake, "--build", "build", "--target",
                                 "OpenEnroth_GameTest", "--parallel", "4"], cwd=ROOT)
        if result.returncode:
            return result.returncode
    if not GAME_TEST.is_file():
        print("GameTest binary is missing; build the target first.", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="mm6-firstmap-gate-") as first:
        profile = Path(first)
        os.chmod(profile, 0o700)
        for name, flag, value in SEVEN_CHILD_CHAIN:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"]):
                return 1
    with tempfile.TemporaryDirectory(prefix="mm6-firstmap-fault-") as second:
        profile = Path(second)
        os.chmod(profile, 0o700)
        for name, flag, value in [SEED, V4, V5_WRITE, *FAULT_CHILDREN]:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"]):
                return 1
    if not run_child(MM7_CONTROL, None, None, ROOT,
                     paths["MM7_DATA_PATH"]):
        return 1
    print("PASS: seven-child MM6 chain, both full-retry fault children, and MM7 control")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
