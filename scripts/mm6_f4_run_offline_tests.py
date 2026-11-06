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
    subprocess.run([sys.executable, "-B", str(root / "tools/mm6_map_name_manifest.py"),
                    "--research", str(research)], check=True)
    for script, source in (("mm6_f4_check_spell_items.py", research),
                           ("mm6_f4_check_actor_descriptors.py", research),
                           ("mm6_f4_check_item_footprints.py", research / "icons"),
                           ("mm6_f4_check_award_event_operands.py", research),
                           ("mm6_f4_check_day_of_year_operands.py", research),
                           ("mm6_f4_check_gold_event_operands.py", research),
                           ("mm6_f4_check_npc_event_operands.py", research)):
        subprocess.run([sys.executable, "-B", str(root / "scripts" / script), str(source)], check=True)

    rules = Path.home() / "Library/Application Support/Enroth Continuum/Profiles/MM6/rules"
    creation_manifest = rules / "creation.json"
    transport_manifest = rules / "transport.json"
    spell_manifest = rules / "spells.json"
    first_aid_manifest = rules / "first-aid.json"
    power_cure_manifest = rules / "power-cure.json"
    timed_cure_manifest = rules / "timed-cures.json"
    combat_buff_manifest = rules / "combat-buffs.json"
    house_room_manifest = rules / "house-rooms.json"
    if not all(path.is_file() for path in (creation_manifest, transport_manifest,
                                           spell_manifest, first_aid_manifest, power_cure_manifest,
                                           timed_cure_manifest, combat_buff_manifest,
                                           house_room_manifest)):
        parser.error("private MM6 rule or room manifest is missing from the local user profile")
    subprocess.run([sys.executable, "-B", str(root / "scripts/mm6_f4_check_spell_learning.py"),
                    str(research), str(creation_manifest)], check=True)

    environment = os.environ.copy()
    environment.update({
        "MM6_RETAINED_EVT_DIR": str(research / "icons"),
        "MM6_ACTIVE_EVT_LIST": str(research / "F4_ACTIVE_EVT_LIST_2026-11-06.txt"),
        "MM6_F3_RETAINED_USEITEMS": str(research / "icons/useitems.txt"),
        "ENROTH_MM6_INDOOR_DECODED_DIR": str(research / "F4_INDOOR_DECODED"),
        "ENROTH_MM6_OUTDOOR_DECODED_DIR": str(research / "F4_OUTDOOR_DECODED"),
        "ENROTH_MM6_F3_RESEARCH_DIR": str(research),
        "ENROTH_MM6_TRANSPORT_MANIFEST": str(transport_manifest),
        "ENROTH_MM6_SPELL_RULES_MANIFEST": str(spell_manifest),
        "ENROTH_MM6_FIRST_AID_MANIFEST": str(first_aid_manifest),
        "ENROTH_MM6_POWER_CURE_MANIFEST": str(power_cure_manifest),
        "ENROTH_MM6_TIMED_CURE_MANIFEST": str(timed_cure_manifest),
        "ENROTH_MM6_COMBAT_BUFF_MANIFEST": str(combat_buff_manifest),
        "ENROTH_MM6_ROOM_MANIFEST": str(house_room_manifest),
    })
    subprocess.run([str(binary), "--gtest_filter=MM6*.*:LodReader.OriginalDirectoryOrder"],
                   check=True, env=environment)


if __name__ == "__main__":
    main()
