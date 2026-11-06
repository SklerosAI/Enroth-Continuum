#!/usr/bin/env python3
"""Reconcile retained MM6 media names with shipped tables and active events.

Only the already extracted, Git-ignored reference set is read. The JSON report
must also stay inside that directory; no media bytes or owned names enter Git.
"""

import argparse
import csv
import hashlib
import json
import re
import struct
from collections import Counter
from pathlib import Path


def text_rows(path: Path) -> list[list[str]]:
    return list(csv.reader(path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t"))


def entry_names(path: Path) -> set[str]:
    return {name.strip().casefold() for name in path.read_text(encoding="cp1252").splitlines() if name.strip()}


def sound_rows(path: Path) -> list[dict]:
    data = path.read_bytes()
    if len(data) < 4:
        raise ValueError("dsounds.bin has no record count")
    count = struct.unpack_from("<I", data)[0]
    if len(data) != 4 + count * 112:
        raise ValueError("dsounds.bin does not end at its declared record boundary")
    rows = []
    for index in range(count):
        start = 4 + index * 112
        name = data[start:start + 32].split(b"\0", 1)[0].decode("cp1252", errors="replace")
        rows.append({"index": index, "name": name, "id": struct.unpack_from("<I", data, start + 32)[0]})
    return rows


def active_event_media(icons: Path, map_stems: set[str], bitmaps: set[str]) -> dict:
    sound_ids = Counter()
    opcode11 = []
    opcode12 = []
    failures = []
    for path in sorted(icons.glob("*.evt")):
        if path.stem.casefold() not in map_stems | {"global"}:
            continue
        data = path.read_bytes()
        offset = 0
        while offset < len(data):
            size = data[offset] + 1
            if size < 5 or offset + size > len(data):
                failures.append({"file": path.name, "offset": offset, "reason": "invalid frame"})
                break
            event_id = struct.unpack_from("<H", data, offset + 1)[0]
            opcode = data[offset + 4]
            payload = data[offset + 5:offset + size]
            if opcode == 3:
                if len(payload) == 12:
                    sound_ids[struct.unpack_from("<I", payload)[0]] += 1
                else:
                    failures.append({"file": path.name, "offset": offset, "reason": "unexpected opcode-3 length"})
            elif opcode in (11, 12):
                fixed_bytes = 4 if opcode == 11 else 8
                if len(payload) <= fixed_bytes or payload[-1] != 0:
                    failures.append({"file": path.name, "offset": offset,
                                     "reason": f"unexpected opcode-{opcode} shape"})
                else:
                    name = payload[fixed_bytes:-1].decode("cp1252", errors="replace")
                    record = {"file": path.name, "event_id": event_id,
                              "offset": offset, "payload_bytes": len(payload),
                              "bitmap_candidate": name,
                              "bitmap_name_present": name.casefold() in bitmaps,
                              "payload_sha256": hashlib.sha256(payload).hexdigest()}
                    if opcode == 11:
                        record["object_id"] = struct.unpack_from("<I", payload)[0]
                        opcode11.append(record)
                    else:
                        record["object_id"], record["face_id"] = struct.unpack_from("<II", payload)
                        opcode12.append(record)
            offset += size
    return {"event_sound_uses": dict(sorted(sound_ids.items())),
            "opcode11_records": opcode11, "opcode12_records": opcode12,
            "framing_failures": failures}


def inherited_room_names(path: Path) -> list[str]:
    source = path.read_text(encoding="utf-8")
    start = source.index("pAnimatedRooms =")
    end = source.index("} };", start)
    rows = re.findall(r'^\s*\{\s*"([^"]*)"\s*,', source[start:end], re.MULTILINE)
    if len(rows) != 196:
        raise ValueError(f"expected 196 inherited room entries, found {len(rows)}")
    return rows


def literal_movie_calls(source_root: Path) -> set[str]:
    """Collect startup and direct movie literals; event-provided names are separate."""
    fsm = (source_root / "src/Application/GameStates/GameFsmBuilder.cpp").read_text(encoding="utf-8")
    game = (source_root / "src/Application/Game.cpp").read_text(encoding="utf-8")
    intro = re.findall(r'\.state<VideoState>\([^\n]*?,\s*"([^"]+)"\)', fsm)
    direct = re.findall(r'PlayFullscreenMovie\("([^"]+)"\)', game)
    return {name.casefold() for name in intro + direct}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="output JSON inside that directory")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents:
        parser.error("report must remain in the private reference directory")
    icons = root / "icons"
    required = [root / name for name in ("Anims1.entries", "Anims2.entries", "Audio.entries",
                                           "BITMAPS.entries", "music-manifest.json")]
    required += [icons / name for name in ("mapstats.txt", "2devents.txt", "dsounds.bin")]
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        parser.error("missing retained references: " + ", ".join(missing))

    videos = entry_names(root / "Anims1.entries") | entry_names(root / "Anims2.entries")
    audio = entry_names(root / "Audio.entries")
    bitmaps = entry_names(root / "BITMAPS.entries")
    soundtrack = json.loads((root / "music-manifest.json").read_text(encoding="utf-8"))
    track_files = {int(row["file"][:-4]) for row in soundtrack
                   if re.fullmatch(r"[0-9]+\.mp3", row["file"], re.IGNORECASE)}
    maps = [row for row in text_rows(icons / "mapstats.txt") if len(row) > 25 and row[0].strip().isdigit()]
    map_tracks = {int(row[25]) for row in maps if row[25].strip().isdigit()}
    map_stems = {Path(row[2].strip().strip('"')).stem.casefold() for row in maps}
    houses = [row for row in text_rows(icons / "2devents.txt") if len(row) > 4 and row[0].strip().isdigit()]
    room_ids = {int(row[4]) for row in houses if row[4].strip().isdigit()}
    source_root = Path(__file__).resolve().parents[1]
    room_source = source_root / "src/GUI/UI/UIHouses.cpp"
    movie_sources = [source_root / "src/Application/GameStates/GameFsmBuilder.cpp",
                     source_root / "src/Application/Game.cpp"]
    rooms = inherited_room_names(room_source)
    literal_movies = literal_movie_calls(source_root)
    outside_rooms = sorted(index for index in room_ids if index < 0 or index >= len(rooms))
    room_video_matches = sorted(index for index in room_ids if index not in outside_rooms
                                and rooms[index].casefold() in videos)

    sounds = sound_rows(icons / "dsounds.bin")
    sound_ids = {row["id"] for row in sounds}
    absent_sound_names = [row for row in sounds if row["name"] and row["name"].casefold() not in audio]
    duplicate_sound_ids = {sound_id: count for sound_id, count in Counter(row["id"] for row in sounds).items()
                           if count > 1}
    event_media = active_event_media(icons, map_stems, bitmaps)
    event_ids_absent = sorted(set(event_media["event_sound_uses"]) - sound_ids)

    summary = {
        "archive_video_names": len(videos), "archive_audio_names": len(audio),
        "video_names_with_bik_or_smk_suffix": sum(name.endswith((".bik", ".smk")) for name in videos),
        "literal_movie_calls": len(literal_movies),
        "literal_movie_base_names_present": sum(name in videos for name in literal_movies),
        "literal_movie_suffix_candidates_present": sum(
            any(f"{name}.{suffix}" in videos for suffix in ("bik", "smk")) for name in literal_movies),
        "maps": len(maps), "map_track_ids": sorted(map_tracks),
        "installed_track_ids": sorted(track_files), "missing_map_tracks": sorted(map_tracks - track_files),
        "houses": len(houses), "distinct_room_ids": len(room_ids),
        "room_ids_outside_inherited_table": outside_rooms,
        "inherited_room_video_matches": len(room_video_matches),
        "sound_table_rows": len(sounds), "sound_ids": len(sound_ids),
        "absent_sound_archive_names": len(absent_sound_names),
        "event_sound_uses": sum(event_media["event_sound_uses"].values()),
        "event_sound_ids_absent_from_table": event_ids_absent,
        "active_opcode11_records": len(event_media["opcode11_records"]),
        "opcode11_bitmap_candidates_present": sum(row["bitmap_name_present"] for row in event_media["opcode11_records"]),
        "active_opcode12_records": len(event_media["opcode12_records"]),
        "opcode12_bitmap_candidates_present": sum(row["bitmap_name_present"] for row in event_media["opcode12_records"]),
        "event_framing_failures": event_media["framing_failures"],
    }
    result = {"format": 1, "summary": summary, "absent_sound_names": absent_sound_names,
              "duplicate_sound_ids": duplicate_sound_ids,
              "literal_movie_names": sorted(literal_movies),
              "room_video_matching_ids": room_video_matches, "event_media": event_media,
              "input_sha256": {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
                               for path in required},
              "source_sha256": {str(path.relative_to(source_root)): hashlib.sha256(path.read_bytes()).hexdigest()
                                for path in [room_source, *movie_sources]}}
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if summary["missing_map_tracks"] or event_ids_absent or event_media["framing_failures"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
