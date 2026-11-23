#!/usr/bin/env python3
"""Run the MM7 Dragoon headless regression matrix using private local data."""
import argparse
import os
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-path", required=True, type=Path)
    parser.add_argument("--binary", type=Path, default=Path("build/test/Bin/GameTest/OpenEnroth_GameTest"))
    parser.add_argument("--test-path", type=Path, default=Path("test/Data"))
    args = parser.parse_args()
    command = [str(args.binary.resolve()), "--game", "mm7", "--headless",
               "--data-path", str(args.data_path.resolve()), "--test-path", str(args.test_path.resolve())]
    environment = dict(os.environ)
    environment.pop("ENROTH_MM7_MOD_DIR", None)
    flags = ["ENROTH_MM7_DRAGOON_CLASS", "ENROTH_MM7_ALL_ARTIFACTS_STORAGE",
             "ENROTH_MM7_CHEST_REROLL", "ENROTH_MM7_MONSTER_LOOT_REROLL",
             "ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"]
    for flag in flags:
        environment[flag] = "0"
    subprocess.run(command + ["--gtest_filter=*-MM6*.*"], env=environment, check=True)
    environment[flags[0]] = "1"
    subprocess.run(command + ["--gtest_filter=MM7*.*"], env=environment, check=True)
    for flag in flags:
        environment[flag] = "1"
    coexistence = ("MM7PrivateMods.*:MM7PrivatePortraits.*:MM7AllArtifactsEngine.*:"
                   "MM7MonsterLootReroll.*:MM7ChestReroll.*:MM7FortunesTurn.*:MM7HarmondaleStorage.*")
    subprocess.run(command + ["--gtest_filter=" + coexistence], env=environment, check=True)


if __name__ == "__main__":
    main()
