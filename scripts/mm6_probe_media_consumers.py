#!/usr/bin/env python3
"""Compare retained MM6 presentation indexes with current portrait/font/voice consumers.

This F2 source/index probe writes exact names and IDs only to an ignored local
report. Name matches and possible generated sound IDs do not prove playback.
"""

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


SOURCE_FILES = (
    "scripts/mm6_probe_media_consumers.py",
    "src/Engine/mm7_data.cpp",
    "src/Engine/mm7_data.h",
    "src/GUI/UI/UIPartyCreation.cpp",
    "src/GUI/UI/UIGame.cpp",
    "src/Engine/Objects/Character.cpp",
    "src/Engine/Spells/Spells.cpp",
    "src/GUI/GUIWindow.cpp",
    "src/GUI/UI/UIBooks.cpp",
    "src/GUI/UI/UICredits.cpp",
    "src/Engine/Engine.cpp",
    "src/Media/Audio/AudioPlayer.cpp",
    "src/Media/MediaPlayer.cpp",
    "src/Library/Vid/VidReader.cpp",
    "src/Library/Vid/VidSnapshots.h",
    "src/Library/Snd/SndReader.cpp",
    "src/Library/Snd/SndSnapshots.h",
)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def entry_names(path: Path) -> set[str]:
    return {name.strip().casefold() for name in path.read_text(encoding="cp1252").splitlines() if name.strip()}


def sound_ids(path: Path) -> set[int]:
    data = path.read_bytes()
    if len(data) < 4:
        raise ValueError("short sound table")
    count = struct.unpack_from("<I", data)[0]
    if len(data) != 4 + 112 * count:
        raise ValueError("sound table row width/boundary changed")
    return {struct.unpack_from("<I", data, 4 + 112 * i + 32)[0] for i in range(count)}


def portrait_and_voice_data(path: Path) -> tuple[list[str], list[list[int]], set[int]]:
    source = path.read_text(encoding="utf-8")
    portrait_section = source.split("pPlayerPortraitsNames = {{", 1)[1].split("}};", 1)[0]
    portraits = re.findall(r'"([^"]+)"', portrait_section)
    voice_section = source.split("byte_4ECF08 = {{", 1)[1].split("}};", 1)[0]
    voice_counts = [list(map(int, re.findall(r"\d+", row)))
                    for row in re.findall(r"\{\{([^{}]+)\}\}", voice_section)]
    speech_section = source.split("speechVariants = {", 1)[1].split("portraitVariants", 1)[0]
    variants = {int(value)
                for body in re.findall(r"\{SPEECH_[^,]+,\s*\{([^}]+)\}", speech_section)
                for value in re.findall(r"\d+", body) if int(value) > 0}
    if len(portraits) != 25 or len(voice_counts) != 48 or {len(row) for row in voice_counts} != {25}:
        raise ValueError("inherited portrait/voice table shape changed")
    if not variants or max(variants) > 48:
        raise ValueError("inherited speech variant range changed")
    return portraits, voice_counts, variants


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored retained MM6 research directory")
    parser.add_argument("report", type=Path, help="detailed JSON inside that directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    output = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must be an ignored MM6 research directory")
    if not output.is_relative_to(root) or output.suffix.lower() != ".json" or not output.parent.is_dir():
        parser.error("report must be JSON inside the ignored reference directory")

    input_paths = {"icons": root / "ICONS.entries", "sounds": root / "icons/dsounds.bin"}
    source_paths = {name: repo / name for name in SOURCE_FILES}
    missing = [str(path) for path in (*input_paths.values(), *source_paths.values()) if not path.is_file()]
    if missing:
        parser.error("missing retained index or source: " + ", ".join(missing))

    icons = entry_names(input_paths["icons"])
    sounds = sound_ids(input_paths["sounds"])
    portraits, voice_counts, variants = portrait_and_voice_data(source_paths["src/Engine/mm7_data.cpp"])
    creation = source_paths["src/GUI/UI/UIPartyCreation.cpp"].read_text(encoding="utf-8")
    game_ui = source_paths["src/GUI/UI/UIGame.cpp"].read_text(encoding="utf-8")
    reaction = source_paths["src/Engine/Objects/Character.cpp"].read_text(encoding="utf-8")
    if "for (int uX = 0; uX < 22; ++uX)" not in creation or "for (unsigned j = 0; j < 56; ++j)" not in game_ui:
        raise ValueError("creation/runtime portrait loop bounds changed")
    if "2 * (pickedVariant + 50 * uVoiceID) + 4998" not in reaction:
        raise ValueError("voice sound-ID formula changed")

    class_icons = re.findall(r'ui_partycreation_class_icons\[\d+\] = assets->getIcon\("([^"]+)"\)', creation)
    if len(class_icons) != 9:
        raise ValueError("inherited creation class-icon list changed")
    literal_creation_icons = sorted(set(re.findall(r'assets->getIcon\("([^"]+)"\)', creation)))
    creation_portraits = [f"{name}01" for name in portraits[:22]]
    runtime_portraits = [f"{name}{frame:02}" for name in portraits[:22] for frame in range(1, 57)]
    arrows = [f"{side}{index}" for side in ("arrowl", "arrowr") for index in range(1, 20)]

    font_uses = {}
    for path in sorted((repo / "src").rglob("*.cpp")):
        names = sorted(set(re.findall(r'LoadFont\("([^"]+\.fnt)"\)', path.read_text(encoding="utf-8", errors="replace"))))
        if names:
            font_uses[str(path.relative_to(repo))] = names
    font_names = {name for names in font_uses.values() for name in names}
    retained_fonts = {name for name in icons if name.endswith(".fnt")}

    # This is an upper bound over inherited nonzero speech variants and the 22
    # portrait slots offered by current creation UI, not observed reactions.
    voice_details = {}
    possible_voice_ids = set()
    for voice in range(22):
        generated = {4998 + 2 * (variant + 50 * voice) + subvariant
                     for variant in variants for subvariant in range(voice_counts[variant - 1][voice])}
        possible_voice_ids.update(generated)
        voice_details[str(voice)] = {"possible_sound_ids": sorted(generated),
                                      "present_sound_ids": sorted(generated & sounds),
                                      "absent_sound_ids": sorted(generated - sounds)}

    def matched(names: list[str] | set[str]) -> list[str]:
        return sorted(name for name in names if name.casefold() in icons)

    def absent(names: list[str] | set[str]) -> list[str]:
        return sorted(name for name in names if name.casefold() not in icons)

    summary = {
        "retained_icon_entries": len(icons),
        "inherited_creation_class_icons": len(class_icons),
        "creation_class_icons_present": len(matched(class_icons)),
        "literal_creation_icons": len(literal_creation_icons),
        "literal_creation_icons_present": len(matched(literal_creation_icons)),
        "creation_portrait_first_frames_requested": len(creation_portraits),
        "creation_portrait_first_frames_present": len(matched(creation_portraits)),
        "possible_runtime_portrait_frames_requested": len(runtime_portraits),
        "possible_runtime_portrait_frames_present": len(matched(runtime_portraits)),
        "creation_arrow_icons_requested": len(arrows),
        "creation_arrow_icons_present": len(matched(arrows)),
        "literal_font_names_requested": len(font_names),
        "literal_font_names_present": len(font_names & retained_fonts),
        "retained_font_entries": len(retained_fonts),
        "retained_font_entries_without_literal_LoadFont": len(retained_fonts - {name.casefold() for name in font_names}),
        "inherited_nonzero_speech_variant_ids": len(variants),
        "possible_voice_sound_ids_for_creation_voices": len(possible_voice_ids),
        "possible_voice_sound_ids_in_mm6_table": len(possible_voice_ids & sounds),
        "possible_voice_sound_ids_absent_from_mm6_table": len(possible_voice_ids - sounds),
    }
    result = {
        "format": 1,
        "scope": "retained MM6 icon/sound indexes and current inherited source only; no original archives or runtime opened",
        "limits": "Literal names and generated-ID potential are lexical checks, not original MM6 asset mapping or audible/visible play.",
        "summary": summary,
        "creation_class_icons_present": matched(class_icons),
        "creation_class_icons_absent": absent(class_icons),
        "literal_creation_icons_present": matched(literal_creation_icons),
        "literal_creation_icons_absent": absent(literal_creation_icons),
        "creation_portrait_first_frames_absent": absent(creation_portraits),
        "runtime_portrait_frames_absent": absent(runtime_portraits),
        "creation_arrow_icons_absent": absent(arrows),
        "font_uses": font_uses,
        "retained_fonts_without_literal_LoadFont": sorted(retained_fonts - {name.casefold() for name in font_names}),
        "voice_possible_by_creation_slot": voice_details,
        "input_sha256": {name: digest(path) for name, path in input_paths.items()},
        "source_sha256": {name: digest(path) for name, path in source_paths.items()},
    }
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
