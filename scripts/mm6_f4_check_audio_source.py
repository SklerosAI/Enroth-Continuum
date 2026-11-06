#!/usr/bin/env python3
"""Verify the staged MM6 SND and retained event-sound table against accepted F3 evidence."""

import argparse
import hashlib
import json
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("research", type=Path)
    parser.add_argument("staged_audio", type=Path)
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    research = args.research.resolve(strict=True)
    if research == allowed or not research.is_relative_to(allowed):
        parser.error("research must be an ignored MM6 reference directory")
    expected_audio = (Path.home() / "Library/Application Support/Enroth Continuum"
                      / "Game Data/MM6/app/Sounds/Audio.snd")
    if args.staged_audio != expected_audio or args.staged_audio.is_symlink():
        parser.error("audio archive must be the staged owner-local MM6 app copy")

    gate = json.loads((research / "F3_GATE_REPORT_HASHES_2026-11-06.json").read_text())
    identity_path = research / "F3_IDENTITY_2026-11-05.json"
    media_path = research / "F3_MEDIA_RECONCILIATION_2026-11-05.json"
    for report in (identity_path, media_path):
        if gate["reports_sha256"][report.name] != sha256(report):
            raise ValueError("MM6 retained F3 media evidence differs from accepted gate")

    identity = json.loads(identity_path.read_text())
    archive = next(row for row in identity["files"]
                   if row["relative_path"] == "Sounds/Audio.snd")
    if (archive["status"] != "match" or
            archive["actual_sha256"] != archive["expected_sha256"] or
            args.staged_audio.stat().st_size != archive["size_bytes"] or
            sha256(args.staged_audio) != archive["expected_sha256"]):
        raise ValueError("staged MM6 audio archive differs from accepted F3 source")

    media = json.loads(media_path.read_text())
    trigger_path = research / "media-trigger-map-private.json"
    if media["source_sha256"][trigger_path.name] != sha256(trigger_path):
        raise ValueError("MM6 event-sound report differs from accepted F3 media source")
    triggers = json.loads(trigger_path.read_text())
    table = research / "icons/dsounds.bin"
    if triggers["input_sha256"]["icons/dsounds.bin"] != sha256(table):
        raise ValueError("MM6 retained sound table differs from accepted event source")
    if len(triggers["event_sounds"]) != 8:
        raise ValueError("MM6 active event-sound count changed")
    print("MM6 audio source: staged SND F3-matched; 8 active event-sound instructions")


if __name__ == "__main__":
    main()
