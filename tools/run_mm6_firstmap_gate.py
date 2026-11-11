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
import shlex
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
GAME_TEST = ROOT / "build/test/Bin/GameTest/OpenEnroth_GameTest"
MM6_APP = ROOT / "build/src/Bin/OpenEnroth/MM6.app/Contents/MacOS/MM6"

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
    ("MM6V5FirstMapAbandonHeadless.DISABLED_PostWorldMismatchThenFullReloadAndInput",
     "ENROTH_MM6_V5_ABANDON_ENABLE", "strict-v5-abandon"),
]
PAPERDOLL_CHILDREN = [
    ("MM6V5PaperdollHeadless.DISABLED_BackpackItemStaysEquippedThroughInventoryCapture",
     "ENROTH_MM6_V5_FIRST_INPUT_ENABLE", "strict-v5-after-action"),
]
NORMAL_SAVE_LOAD = [
    ("MM6V5NormalSaveMenuHeadless.DISABLED_SaveMenuWritesNumberedFirstMapSlot",
     "ENROTH_MM6_V5_NORMAL_SAVE_ENABLE", "strict-v5-normal-save"),
    ("MM6V5NormalLoadMenuHeadless.DISABLED_LoadMenuListsAndLoadsNumberedFirstMapSlot",
     "ENROTH_MM6_V5_NORMAL_LOAD_ENABLE", "strict-v5-normal-load"),
    ("MM6V5NormalLoadMenuHeadless.DISABLED_LoadedSlotSavesAgainThroughSaveMenu",
     "ENROTH_MM6_V5_NORMAL_RESAVE_ENABLE", "strict-v5-normal-resave"),
]
NEW_GAME = [
    ("MM6NewGameHeadless.DISABLED_NewPartySavesFromSaveMenu",
     "ENROTH_MM6_NEW_GAME_ENABLE", "strict-new-game-save"),
    ("MM6NewGameHeadless.DISABLED_NewPartySlotLoadsAndSavesAgain",
     "ENROTH_MM6_NEW_GAME_ENABLE", "strict-new-game-reload"),
]
SAVE_ANYWHERE = [
    ("MM6SaveAnywhereHeadless.DISABLED_DungeonSaveAfterTravel",
     "ENROTH_MM6_SAVE_ANYWHERE_ENABLE", "strict-save-anywhere-write"),
    ("MM6SaveAnywhereHeadless.DISABLED_DungeonSlotLoadsWithItsOutdoorMaps",
     "ENROTH_MM6_SAVE_ANYWHERE_ENABLE", "strict-save-anywhere-reload"),
]
AUTOSAVE = [
    ("MM6AutosaveHeadless.DISABLED_EdgeAndDoorWriteTheAutosave",
     "ENROTH_MM6_AUTOSAVE_ENABLE", "strict-autosave-write"),
]
MM7_CONTROLS = ["Prs.Pr2354", "Prs.Pr2956", "Issues.Issue840"]
PASS_LINE = re.compile(r"\[\s*PASSED\s*\]\s+1 test\b")


CHILD_TIMEOUT = int(os.environ.get("ENROTH_GATE_CHILD_TIMEOUT", "240"))


def run_child(name, flag, value, profile, data, rules=None, timeout=None):
    timeout = max(timeout or 0, CHILD_TIMEOUT)
    env = os.environ.copy()
    for key in list(env):
        if key.startswith("ENROTH_MM6_V") or key in ("ENROTH_MM6_RESTART_PROFILE", "ENROTH_MM6_NEW_GAME_ENABLE",
                                                     "ENROTH_MM6_SAVE_ANYWHERE_ENABLE", "ENROTH_MM6_AUTOSAVE_ENABLE"):
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
                                text=True, timeout=timeout, check=False)
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
    parser.add_argument("--skip-build", action="store_true", help="use existing GameTest and MM6 app binaries")
    parser.add_argument("--stage-f5a-profile", action="store_true",
                        help="prepare and retain an isolated first-map Load slot for a later visible MM6 check")
    args = parser.parse_args()
    if args.list:
        for name, _, _ in SEVEN_CHILD_CHAIN:
            print("seven-child:", name)
        for name, _, _ in FAULT_CHILDREN:
            print("fault-retry:", name)
        for name, _, _ in PAPERDOLL_CHILDREN:
            print("paperdoll:", name)
        for name, _, _ in NORMAL_SAVE_LOAD:
            print("normal save/load:", name)
        for name, _, _ in NEW_GAME:
            print("new game:", name)
        for name, _, _ in SAVE_ANYWHERE:
            print("save anywhere:", name)
        for name, _, _ in AUTOSAVE:
            print("autosave:", name)
        for name in MM7_CONTROLS:
            print("MM7 control:", name)
        return 0

    paths = {}
    required_paths = ("MM6_DATA_PATH", "MM6_RULES_PATH") if args.stage_f5a_profile else (
        "MM6_DATA_PATH", "MM6_RULES_PATH", "MM7_DATA_PATH")
    for key in required_paths:
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
        targets = ["OpenEnroth_GameTest", "MM6"] if args.stage_f5a_profile else ["OpenEnroth_GameTest"]
        result = subprocess.run([cmake, "--build", "build", "--target", *targets, "--parallel", "4"], cwd=ROOT)
        if result.returncode:
            return result.returncode
    if not GAME_TEST.is_file():
        print("GameTest binary is missing; build the target first.", file=sys.stderr)
        return 2
    if args.stage_f5a_profile:
        if not MM6_APP.is_file():
            print("MM6 app binary is missing; build the MM6 target first.", file=sys.stderr)
            return 2
        profile = Path(tempfile.mkdtemp(prefix="mm6-restart-f5a-"))
        os.chmod(profile, 0o700)
        staged = False
        try:
            for name, flag, value in [SEED, V4, V5_WRITE, NORMAL_SAVE_LOAD[0], NORMAL_SAVE_LOAD[1]]:
                if not run_child(name, flag, value, profile,
                                 paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"], timeout=1500):
                    return 1
            slot = profile / "saves/save000.ec6"
            if not slot.is_file() or not slot.stat().st_size:
                print("The staged first-map slot is absent or empty.", file=sys.stderr)
                return 1
            shutil.copytree(paths["MM6_RULES_PATH"], profile / "rules")
            staged = True
        finally:
            if not staged:
                shutil.rmtree(profile)
        print("PASS: isolated first-map slot and fresh-process Load menu check")
        print("Production MM6 New Game remains guarded; select Load and save000.ec6 for the visible check.")
        print("Profile:", profile)
        print("Launch:", shlex.join([str(MM6_APP), "--data-path", str(paths["MM6_DATA_PATH"]),
                                    "--user-path", str(profile)]))
        print("Delete this temporary profile after the check:", shlex.join(["rm", "-r", str(profile)]))
        return 0

    with tempfile.TemporaryDirectory(prefix="mm6-restart-gate-") as first:
        profile = Path(first)
        os.chmod(profile, 0o700)
        for name, flag, value in SEVEN_CHILD_CHAIN:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"]):
                return 1
    with tempfile.TemporaryDirectory(prefix="mm6-restart-fault-") as second:
        profile = Path(second)
        os.chmod(profile, 0o700)
        for name, flag, value in [SEED, V4, V5_WRITE, *FAULT_CHILDREN]:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"]):
                return 1
    with tempfile.TemporaryDirectory(prefix="mm6-restart-paperdoll-") as third:
        profile = Path(third)
        os.chmod(profile, 0o700)
        for name, flag, value in [SEED, V4, V5_WRITE, *PAPERDOLL_CHILDREN]:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"]):
                return 1
    with tempfile.TemporaryDirectory(prefix="mm6-restart-normal-") as third:
        profile = Path(third)
        os.chmod(profile, 0o700)
        for name, flag, value in [SEED, V4]:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"]):
                return 1
        # Every save and every menu listing stages complete first-map sessions. The Load menu child and the resave
        # child load save000.ec6 through the production main-menu load, each in a fresh process.
        for name, flag, value in NORMAL_SAVE_LOAD:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"], timeout=1500):
                return 1
    with tempfile.TemporaryDirectory(prefix="mm6-restart-new-game-") as fifth:
        profile = Path(fifth)
        os.chmod(profile, 0o700)
        # A party made on the creation screen saves from the Save menu, then a fresh process loads, walks and saves
        # again. No seed, v4 or v5 child runs first.
        for name, flag, value in NEW_GAME:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"], timeout=1500):
                return 1
    with tempfile.TemporaryDirectory(prefix="mm6-restart-save-anywhere-") as sixth:
        profile = Path(sixth)
        os.chmod(profile, 0o700)
        # A new party saves on the first map, travels to a neighbouring map and into a dungeon and saves there over the
        # first save. A fresh process loads the dungeon save, walks back out over both outdoor maps and saves again.
        for name, flag, value in SAVE_ANYWHERE:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"], timeout=1500):
                return 1
    with tempfile.TemporaryDirectory(prefix="mm6-restart-autosave-") as seventh:
        profile = Path(seventh)
        os.chmod(profile, 0o700)
        # A new party is autosaved, then autosaved again over a map edge and at a dungeon door.
        for name, flag, value in AUTOSAVE:
            if not run_child(name, flag, value, profile,
                             paths["MM6_DATA_PATH"], paths["MM6_RULES_PATH"], timeout=1500):
                return 1
    for name in MM7_CONTROLS:
        if not run_child(name, None, None, ROOT, paths["MM7_DATA_PATH"]):
            return 1
    print("PASS: seven-child MM6 chain, all full-retry fault children, the paperdoll child, normal Save/Load menus with a "
          "production load and resave, a production New Game saved, loaded and saved again, a dungeon save after travel "
          "loaded with its outdoor maps and saved again, autosaves of a new party and over a map edge and a dungeon "
          "door, and the MM7 controls")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
