#!/usr/bin/env python3
"""Run MM7 campaign chapters in fresh headless processes with kept saves."""

import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


REPO = Path(__file__).resolve().parents[1]
MARKER = ".ec-mm7-disposable-save-gate"
MARKER_TEXT = "EnrothContinuum-MM7-save-gate-v1\n"
SLOT_RE = re.compile(r"save[0-9]{3}\.mm7(?:\.bak|\.passed)?$")
CHAPTER_TIMEOUT_SECONDS = 3 * 60 * 60
LIST_TIMEOUT_SECONDS = 5 * 60


def slot_name(chapter):
    return f"save{chapter:03}.mm7"


def pass_marker_name(chapter):
    return f"{slot_name(chapter)}.passed"


def chapter_files(saves, chapter):
    slot = saves / slot_name(chapter)
    return [slot, saves / f"{slot.name}.bak", saves / pass_marker_name(chapter)]


def validate_profile(root):
    root = root.resolve(strict=True)
    system_temp = Path(tempfile.gettempdir()).resolve(strict=True)
    if root.parent != system_temp or not root.name.startswith("mm7-save-gate-"):
        raise ValueError("Profile must be an MM7 save gate directory under system temp")
    if not root.is_dir() or root.stat().st_mode & 0o077:
        raise ValueError("Profile must be owner-only")
    marker = root / MARKER
    if marker.is_symlink() or not marker.is_file() or marker.read_text() != MARKER_TEXT:
        raise ValueError("Profile marker is missing or invalid")
    for entry in root.iterdir():
        if entry.is_symlink() or entry.name not in {MARKER, "saves", "enroth-continuum-mm7.ini"}:
            raise ValueError("Profile contains an unexpected entry")
    saves = root / "saves"
    if saves.exists():
        if saves.is_symlink() or not saves.is_dir():
            raise ValueError("Profile saves entry is invalid")
        for entry in saves.iterdir():
            if entry.is_symlink() or not entry.is_file():
                raise ValueError("Profile contains an invalid save entry")
            if entry.name not in {"autosave.mm7", "autosave.mm7.bak"}:
                match = SLOT_RE.fullmatch(entry.name)
                if not match or int(entry.name[4:7]) > 7:
                    raise ValueError("Profile contains an unexpected save")
    return root


def digest(path):
    hash_value = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hash_value.update(chunk)
    return hash_value.hexdigest()


def is_passed(saves, chapter):
    slot = saves / slot_name(chapter)
    marker = saves / pass_marker_name(chapter)
    return slot.is_file() and marker.is_file() and marker.read_text() == digest(slot) + "\n"


def passed_chapters(root):
    saves = root / "saves"
    passed = 0
    while passed < 8 and is_passed(saves, passed):
        passed += 1
    return passed


def remove_unverified(root, chapter):
    saves = root / "saves"
    for later in range(chapter + 1, 8):
        if any(path.exists() for path in chapter_files(saves, later)):
            raise ValueError(f"C{later} has files although C{chapter} has no pass marker")
    if chapter == 0:
        if any(entry.name != MARKER for entry in root.iterdir()):
            shutil.rmtree(saves, ignore_errors=True)
            (root / "enroth-continuum-mm7.ini").unlink(missing_ok=True)
            print("C0: has no pass marker and needs a fresh profile, profile reset to its marker")
        return
    removed = [path.name for path in chapter_files(saves, chapter) if path.exists()]
    for path in chapter_files(saves, chapter):
        path.unlink(missing_ok=True)
    if removed:
        print(f"C{chapter}: has no pass marker, removed unverified {', '.join(removed)}")


def chapter_test_name(chapter):
    return f"MM7CampaignChain.DISABLED_Chapter{chapter}"


def require_chapter_tests(args):
    command = [
        str(args.game_test),
        "--game", "mm7",
        "--headless",
        "--gtest_list_tests",
        "--gtest_also_run_disabled_tests",
        "--gtest_filter=MM7CampaignChain.*",
    ]
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
                            timeout=LIST_TIMEOUT_SECONDS)
    if result.returncode:
        print(result.stdout, file=sys.stderr)
        raise RuntimeError(f"Listing chapter tests failed with exit {result.returncode}")
    listed = set()
    suite = ""
    for line in result.stdout.splitlines():
        if line and not line.startswith(" "):
            suite = line.strip()
        elif line.strip():
            listed.add(suite + line.split("#")[0].strip())
    for chapter in range(args.through + 1):
        if chapter_test_name(chapter) not in listed:
            raise RuntimeError(f"C{chapter} test not implemented")


def run_chapter(args, root, chapter):
    saves = root / "saves"
    earlier = {i: digest(saves / slot_name(i)) for i in range(chapter)}
    command = [
        str(args.game_test),
        "--game", "mm7",
        "--headless",
        "--data-path", str(args.data_path),
        "--test-path", str(args.test_path),
        "--mm7-disposable-profile", str(root),
        f"--gtest_filter={chapter_test_name(chapter)}",
        "--gtest_also_run_disabled_tests",
        "--gtest_brief=1",
        "--log-level=critical",
    ]
    fresh_hint = " C0 needs a fresh profile, rerun with --profile to reset it." if chapter == 0 else ""
    try:
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
                                timeout=CHAPTER_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired as error:
        print(error.stdout or "", file=sys.stderr)
        raise RuntimeError(f"C{chapter} timed out after {CHAPTER_TIMEOUT_SECONDS} seconds.{fresh_hint}") from error
    if result.returncode:
        print(result.stdout, file=sys.stderr)
        raise RuntimeError(f"C{chapter} failed with exit {result.returncode}.{fresh_hint}")
    output = saves / slot_name(chapter)
    if output.is_symlink() or not output.is_file() or output.stat().st_size <= 1024:
        raise RuntimeError(f"C{chapter} did not write a regular chapter save")
    for earlier_chapter, before in earlier.items():
        if digest(saves / slot_name(earlier_chapter)) != before or not is_passed(saves, earlier_chapter):
            raise RuntimeError(f"C{chapter} changed an earlier chapter save or pass marker")
    validate_profile(root)
    (saves / pass_marker_name(chapter)).write_text(digest(output) + "\n")
    print(f"C{chapter}: passed, save and pass marker kept")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-path", type=Path, help="Local MM7 game data")
    parser.add_argument("--game-test", type=Path, default=REPO / "build/test/Bin/GameTest/OpenEnroth_GameTest")
    parser.add_argument("--test-path", type=Path, default=REPO / "test/Data")
    parser.add_argument("--through", type=int, choices=range(8), default=0)
    parser.add_argument("--profile", type=Path, help="Resume a marked profile from a previous run")
    parser.add_argument("--remove-profile", action="store_true", help="Remove a marked profile without running chapters")
    args = parser.parse_args()

    if args.remove_profile:
        if args.profile is None:
            parser.error("--remove-profile requires --profile")
        root = validate_profile(args.profile)
        shutil.rmtree(root)
        print("Marked profile removed")
        return 0
    if args.data_path is None:
        parser.error("--data-path is required")
    if not args.game_test.is_file() or not args.test_path.is_dir() or not args.data_path.is_dir():
        raise ValueError("GameTest binary, test path, or MM7 data path is missing")
    require_chapter_tests(args)
    if args.profile is None:
        root = Path(tempfile.mkdtemp(prefix="mm7-save-gate-"))
        os.chmod(root, 0o700)
        (root / MARKER).write_text(MARKER_TEXT)
    else:
        root = validate_profile(args.profile)
    print(f"Profile: {root}")
    start = passed_chapters(root)
    if start > args.through:
        print(f"C0-C{args.through}: already passed")
        return 0
    remove_unverified(root, start)
    for chapter in range(start, args.through + 1):
        run_chapter(args, root, chapter)
    if passed_chapters(root) <= args.through:
        raise RuntimeError(f"C0-C{args.through} do not all have pass markers")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"MM7 campaign chain: {error}", file=sys.stderr)
        sys.exit(1)
