#!/usr/bin/env python3
"""Write a private MM6 media trigger map from retained extracts only.

This never opens original archives or the installed game. Run with the ignored
research directory as both input owner and report parent.
"""

import argparse
import hashlib
import json
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

sys.dont_write_bytecode = True
from mm6_probe_media_references import entry_names, sound_rows, text_rows


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def event_sounds(icons: Path, map_stems: set[str]) -> tuple[list[dict], list[dict]]:
    uses, failures = [], []
    for path in sorted(icons.glob("*.evt")):
        if path.stem.casefold() not in map_stems | {"global"}:
            continue
        data = path.read_bytes()
        offset = 0
        while offset < len(data):
            size = data[offset] + 1
            if size < 5 or offset + size > len(data):
                failures.append({"file": path.name, "offset": offset, "reason": "frame"})
                break
            if data[offset + 4] == 3:
                payload = data[offset + 5:offset + size]
                if len(payload) != 12:
                    failures.append({"file": path.name, "offset": offset, "reason": "opcode-3 shape"})
                else:
                    uses.append({"file": path.name, "event_id": struct.unpack_from("<H", data, offset + 1)[0],
                                 "offset": offset, "sound_id": struct.unpack_from("<I", payload)[0],
                                 "payload_sha256": hashlib.sha256(payload).hexdigest()})
            offset += size
    return uses, failures


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents or "local-game-data" not in root.parts:
        parser.error("input and output must stay in the ignored local-game-data reference directory")
    icons = root / "icons"
    inputs = [root / name for name in ("Anims1.entries", "Anims2.entries", "Audio.entries",
                                      "music-manifest.json", "media-reference-probe.json")]
    inputs += [icons / name for name in ("mapstats.txt", "2devents.txt", "dsounds.bin")]
    if any(not path.is_file() for path in inputs):
        parser.error("missing retained reference or earlier F2 report")

    video = entry_names(root / "Anims1.entries") | entry_names(root / "Anims2.entries")
    audio = entry_names(root / "Audio.entries")
    tracks = {int(row["file"][:-4]) for row in json.loads((root / "music-manifest.json").read_text())
              if re.fullmatch(r"\d+\.mp3", row["file"], re.I)}
    maps = [row for row in text_rows(icons / "mapstats.txt") if len(row) > 25 and row[0].strip().isdigit()]
    map_rows = [{"row_id": int(row[0]), "map": row[2].strip().strip('"'), "music_id": int(row[25]),
                 "track_present": int(row[25]) in tracks} for row in maps if row[25].strip().isdigit()]
    map_stems = {Path(row["map"]).stem.casefold() for row in map_rows}
    house_rows_raw = [row for row in text_rows(icons / "2devents.txt") if len(row) > 4 and row[0].strip().isdigit()]
    house_rows = [{"house_id": int(row[0]), "room_id": int(row[4]) if row[4].strip().isdigit() else None}
                  for row in house_rows_raw]
    sounds = sound_rows(icons / "dsounds.bin")
    by_id = defaultdict(list)
    for row in sounds:
        by_id[row["id"]].append({**row, "archive_name_present": row["name"].casefold() in audio})
    uses, failures = event_sounds(icons, map_stems)
    for use in uses:
        use["sound_table_matches"] = by_id.get(use["sound_id"], [])
    spell_source = (Path(__file__).resolve().parents[1] / "src/Engine/Spells/Spells.cpp").read_text()
    spell_block = spell_source.split("SpellSoundIds = {{", 1)[1].split("}};", 1)[0]
    inherited_spell_sounds = []
    for spell, base_text in re.findall(r"\{(SPELL_[A-Z0-9_]+),\s*(\d+)\}", spell_block):
        base = int(base_text)
        inherited_spell_sounds.append({"spell": spell, "cast_id": base, "impact_id": base + 1,
            "cast_table_matches": by_id.get(base, []), "impact_table_matches": by_id.get(base + 1, [])})
    room_source_text = (Path(__file__).resolve().parents[1] / "src/GUI/UI/UIHouses.cpp").read_text()
    room_block = room_source_text.split("pAnimatedRooms =", 1)[1].split("} };", 1)[0]
    inherited_rooms = re.findall(r'^\s*\{\s*"([^"]*)"\s*,', room_block, re.M)
    if len(inherited_rooms) != 196:
        parser.error("inherited room table shape changed")
    room_projection = [{"room_id": room_id, "inherited_video_name": inherited_rooms[room_id],
                        "inherited_video_in_mm6_index": inherited_rooms[room_id].casefold() in video}
                       for room_id in sorted({row["room_id"] for row in house_rows if row["room_id"] is not None})
                       if 0 <= room_id < len(inherited_rooms)]
    earlier = json.loads((root / "media-reference-probe.json").read_text())
    movie_literals = [{"name": name, "raw_index_present": name in video,
                       "suffix_lookup_present": any(f"{name}.{ext}" in video for ext in ("smk", "bik"))}
                      for name in earlier["literal_movie_names"]]
    source_root = Path(__file__).resolve().parents[1]
    source_paths = ["src/Media/MediaPlayer.cpp", "src/Media/Audio/AudioPlayer.cpp",
                    "src/GUI/UI/UIHouses.cpp", "src/GUI/UI/UITransition.cpp",
                    "src/Engine/Evt/EvtInterpreter.cpp", "src/Engine/Evt/EvtInstruction.cpp",
                    "src/Engine/Spells/Spells.cpp", "src/Application/GameStates/GameFsmBuilder.cpp",
                    "src/Application/Game.cpp"]
    summary = {"maps": len(map_rows), "map_track_ids": len({row["music_id"] for row in map_rows}),
               "map_tracks_missing": sum(not row["track_present"] for row in map_rows),
               "houses": len(house_rows), "houses_with_numeric_room": sum(row["room_id"] is not None for row in house_rows),
               "distinct_house_rooms": len({row["room_id"] for row in house_rows if row["room_id"] is not None}),
               "event_sound_uses": len(uses), "event_sound_ids": len({row["sound_id"] for row in uses}),
               "event_sounds_without_table": sum(not row["sound_table_matches"] for row in uses),
               "event_sounds_without_archive_name": sum(not row["sound_table_matches"] or
                   not any(match["archive_name_present"] for match in row["sound_table_matches"]) for row in uses),
               "inherited_spell_sound_rows": len(inherited_spell_sounds),
               "inherited_spell_cast_ids_in_mm6_table": sum(bool(row["cast_table_matches"]) for row in inherited_spell_sounds),
               "inherited_spell_impact_ids_in_mm6_table": sum(bool(row["impact_table_matches"]) for row in inherited_spell_sounds),
               "inherited_house_video_matches": sum(row["inherited_video_in_mm6_index"] for row in room_projection),
               "event_framing_failures": len(failures),
               "movie_literals": len(movie_literals), "movie_literals_raw_index_present": sum(row["raw_index_present"] for row in movie_literals),
               "movie_literals_suffix_lookup_present": sum(row["suffix_lookup_present"] for row in movie_literals),
               "raw_video_index_names": len(video)}
    result = {"format": 1, "summary": summary, "maps": map_rows, "houses": house_rows,
              "event_sounds": uses, "event_framing_failures": failures, "movie_literals": movie_literals,
              "all_video_index_names": sorted(video), "sound_table": sounds,
              "inherited_spell_sounds": inherited_spell_sounds, "inherited_room_projection": room_projection,
              "input_sha256": {str(path.relative_to(root)): digest(path) for path in inputs},
              "source_sha256": {name: digest(source_root / name) for name in source_paths}}
    report.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    if failures or summary["map_tracks_missing"] or summary["event_sounds_without_table"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
