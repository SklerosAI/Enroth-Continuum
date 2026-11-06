#!/usr/bin/env python3
"""Run the MM6 F4 native suite with private, F3-matched retained fixtures.

This reads only ignored retained extracts and the MM6 user-profile manifest.
It never starts the game or opens the original GOG installation.
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--research-dir", type=Path, help="ignored retained research directory")
    parser.add_argument("--unit-binary", type=Path, help="built OpenEnroth unit executable")
    args = parser.parse_args()

    root = Path(__file__).resolve().parent.parent
    research = (args.research_dir or root / "local-game-data/mm6/research-2026-11-05").resolve()
    binary = (args.unit_binary or root / "build/test/Bin/UnitTest/OpenEnroth_UnitTest").resolve()
    if not binary.is_file():
        parser.error("build the native OpenEnroth_UnitTest target first")
    if not research.is_dir() or research.parent.name != "mm6" or research.parent.parent.name != "local-game-data":
        parser.error("research directory must be inside ignored local-game-data/mm6")

    for script in ("mm6_f4_prepare_indoor_fixture.py", "mm6_f4_prepare_outdoor_fixture.py"):
        subprocess.run([sys.executable, "-B", str(root / "scripts" / script), str(research)], check=True)

    manifest = Path.home() / "Library/Application Support/Enroth Continuum/Profiles/MM6/rules/transport.json"
    if not manifest.is_file():
        parser.error("private MM6 transport manifest is missing from the local user profile")

    environment = os.environ.copy()
    environment.update({
        "MM6_RETAINED_EVT_DIR": str(research / "icons"),
        "MM6_ACTIVE_EVT_LIST": str(research / "F4_ACTIVE_EVT_LIST_2026-11-06.txt"),
        "ENROTH_MM6_INDOOR_DECODED_DIR": str(research / "F4_INDOOR_DECODED"),
        "ENROTH_MM6_OUTDOOR_DECODED_DIR": str(research / "F4_OUTDOOR_DECODED"),
        "ENROTH_MM6_TRANSPORT_MANIFEST": str(manifest),
    })
    subprocess.run([str(binary), "--gtest_filter=MM6*.*:LodReader.OriginalDirectoryOrder"],
                   check=True, env=environment)


if __name__ == "__main__":
    main()
