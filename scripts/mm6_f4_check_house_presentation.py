#!/usr/bin/env python3
"""Verify retained MM6 house icons and the staged, F3-matched icon archive."""

import argparse
import hashlib
import json
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("staged_icons", type=Path)
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    research = args.research.resolve(strict=True)
    if research == allowed or not research.is_relative_to(allowed):
        parser.error("research must be an ignored MM6 reference directory")
    expected_manifest = (Path.home() / "Library/Application Support/Enroth Continuum"
                         / "Profiles/MM6/rules/house-rooms.json")
    if args.manifest != expected_manifest or args.manifest.is_symlink():
        parser.error("manifest must be the owner-local MM6 room source")
    expected_icons = (Path.home() / "Library/Application Support/Enroth Continuum"
                      / "Game Data/MM6/app/data/icons.lod")
    if args.staged_icons != expected_icons or args.staged_icons.is_symlink():
        parser.error("icon archive must be the staged owner-local MM6 app copy")

    identity_path = research / "F3_IDENTITY_2026-11-05.json"
    gate = json.loads((research / "F3_GATE_REPORT_HASHES_2026-11-06.json").read_text())
    if gate["reports_sha256"][identity_path.name] != sha256(identity_path):
        raise ValueError("retained MM6 F3 identity report differs from accepted gate")
    identity = json.loads(identity_path.read_text())
    icon_identity = next(row for row in identity["files"]
                         if row["relative_path"] == "data/icons.lod")
    if (icon_identity["status"] != "match" or
            icon_identity["actual_sha256"] != icon_identity["expected_sha256"] or
            args.staged_icons.stat().st_size != icon_identity["size_bytes"] or
            sha256(args.staged_icons) != icon_identity["expected_sha256"]):
        raise ValueError("staged MM6 icon archive differs from accepted F3 source")

    icons_index = research / "icons.entries"
    table = research / "icons/2devents.txt"
    media = json.loads((research / "media-schema-leads-private.json").read_text())
    house = json.loads((research / "house-route-probe.json").read_text())
    manifest = json.loads(args.manifest.read_text())
    if media["input_sha256"]["icons.entries"] != sha256(icons_index):
        raise ValueError("retained MM6 icon index differs from frozen media probe")
    if house["format"] != 2 or house["input_sha256"]["icons/2devents.txt"] != sha256(table):
        raise ValueError("retained MM6 house table differs from frozen house probe")
    if manifest["source_house_table_sha256"] != sha256(table) or len(manifest["rooms"]) != 119:
        raise ValueError("private room manifest differs from accepted house source")

    icon_names = {line.casefold() for line in icons_index.read_text().splitlines()}
    rooms = manifest["rooms"]
    referenced = {row["room_id"] for row in house["house_rows"].values() if row["room_id"] is not None}
    if len(referenced) != 116:
        raise ValueError("MM6 referenced room set changed")
    pictures = 0
    for room_id in referenced:
        picture = rooms[room_id]["npc_picture"]
        if picture:
            pictures += 1
            if f"npc{picture:03d}".casefold() not in icon_names:
                raise ValueError("referenced MM6 proprietor icon is absent")
    if pictures != 51:
        raise ValueError("MM6 referenced proprietor picture count changed")
    print("MM6 house presentation: 116 referenced rooms, 51 proprietor pictures present; staged archive F3-matched")


if __name__ == "__main__":
    main()
