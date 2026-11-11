#!/usr/bin/env python3
"""Run MM7 or MM6 campaign chapters in fresh headless processes with kept saves."""

import argparse
from dataclasses import dataclass
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


REPO = Path(__file__).resolve().parents[1]
CHAPTER_TIMEOUT_SECONDS = 3 * 60 * 60
LIST_TIMEOUT_SECONDS = 5 * 60


@dataclass(frozen=True)
class Title:
    """Where a title's chain keeps its profile and saves, and how its GameTest is started."""
    game: str
    profile_kind: str
    paths: str
    prefix: str
    marker: str
    marker_text: str
    extension: str
    ini: str | None
    chapters: int
    suite: str
    profile_option: str
    data_env: str | None
    rules_env: str | None

    @property
    def slot_re(self):
        return re.compile(rf"save[0-9]{{3}}\.{self.extension}(?:\.bak|\.passed)?$")

    @property
    def autosaves(self):
        return {f"autosave.{self.extension}", f"autosave.{self.extension}.bak"}

    @property
    def root_entries(self):
        return {self.marker, "saves"} | ({self.ini} if self.ini else set())


TITLES = {
    "mm7": Title(game="mm7", profile_kind="an MM7 save gate", paths="MM7 data path", prefix="mm7-save-gate-",
                 marker=".ec-mm7-disposable-save-gate", marker_text="EnrothContinuum-MM7-save-gate-v1\n",
                 extension="mm7", ini="enroth-continuum-mm7.ini", chapters=8, suite="MM7CampaignChain",
                 profile_option="--mm7-disposable-profile", data_env=None, rules_env=None),
    "mm6": Title(game="mm6", profile_kind="an MM6 campaign chain", paths="MM6 data or rules path",
                 prefix="mm6-restart-chain-",  # The MM6 save code takes a GameTest profile root only under this prefix.
                 marker=".ec-mm6-campaign-chain",
                 marker_text="EnrothContinuum-MM6-campaign-chain-v1\n", extension="ec6", ini=None, chapters=10,
                 suite="MM6CampaignChain", profile_option="--mm6-chain-profile", data_env="MM6_DATA_PATH",
                 rules_env="MM6_RULES_PATH"),
}


def slot_name(title, chapter):
    return f"save{chapter:03}.{title.extension}"


def pass_marker_name(title, chapter):
    return f"{slot_name(title, chapter)}.passed"


def chapter_files(title, saves, chapter):
    slot = saves / slot_name(title, chapter)
    return [slot, saves / f"{slot.name}.bak", saves / pass_marker_name(title, chapter)]


def validate_profile(title, root):
    root = root.resolve(strict=True)
    system_temp = Path(tempfile.gettempdir()).resolve(strict=True)
    if root.parent != system_temp or not root.name.startswith(title.prefix):
        raise ValueError(f"Profile must be {title.profile_kind} directory under system temp")
    if not root.is_dir() or root.stat().st_mode & 0o077:
        raise ValueError("Profile must be owner-only")
    marker = root / title.marker
    if marker.is_symlink() or not marker.is_file() or marker.read_text() != title.marker_text:
        raise ValueError("Profile marker is missing or invalid")
    for entry in root.iterdir():
        if entry.is_symlink() or entry.name not in title.root_entries:
            raise ValueError("Profile contains an unexpected entry")
    saves = root / "saves"
    if saves.exists():
        if saves.is_symlink() or not saves.is_dir():
            raise ValueError("Profile saves entry is invalid")
        for entry in saves.iterdir():
            if entry.is_symlink() or not entry.is_file():
                raise ValueError("Profile contains an invalid save entry")
            if entry.name not in title.autosaves:
                match = title.slot_re.fullmatch(entry.name)
                if not match or int(entry.name[4:7]) >= title.chapters:
                    raise ValueError("Profile contains an unexpected save")
    return root


def digest(path):
    hash_value = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hash_value.update(chunk)
    return hash_value.hexdigest()


def is_passed(title, saves, chapter):
    slot = saves / slot_name(title, chapter)
    marker = saves / pass_marker_name(title, chapter)
    return slot.is_file() and marker.is_file() and marker.read_text() == digest(slot) + "\n"


def passed_chapters(title, root):
    saves = root / "saves"
    passed = 0
    while passed < title.chapters and is_passed(title, saves, passed):
        passed += 1
    return passed


def remove_unverified(title, root, chapter):
    saves = root / "saves"
    if chapter == 0:
        if any(entry.name != title.marker for entry in root.iterdir()):
            shutil.rmtree(saves, ignore_errors=True)
            if title.ini:
                (root / title.ini).unlink(missing_ok=True)
            print("C0: has no pass marker and needs a fresh profile, profile reset to its marker")
        return
    for later in range(chapter, title.chapters):
        removed = [path.name for path in chapter_files(title, saves, later) if path.exists()]
        for path in chapter_files(title, saves, later):
            path.unlink(missing_ok=True)
        if removed and later == chapter:
            print(f"C{later}: has no pass marker, removed unverified {', '.join(removed)}")
        elif removed:
            print(f"C{later}: built on unverified C{chapter}, removed {', '.join(removed)}")


def chapter_test_name(title, chapter):
    return f"{title.suite}.DISABLED_Chapter{chapter}"


def require_chapter_tests(title, args):
    command = [
        str(args.game_test),
        "--game", title.game,
        "--headless",
        "--gtest_list_tests",
        "--gtest_also_run_disabled_tests",
        f"--gtest_filter={title.suite}.*",
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
        if chapter_test_name(title, chapter) not in listed:
            raise RuntimeError(f"C{chapter} test not implemented")


def run_chapter(title, args, root, chapter):
    saves = root / "saves"
    earlier = {i: digest(saves / slot_name(title, i)) for i in range(chapter)}
    command = [
        str(args.game_test),
        "--game", title.game,
        "--headless",
        "--data-path", str(args.data_path),
    ]
    if title.rules_env:
        command += ["--rules-path", str(args.rules_path)]
    command += [
        "--test-path", str(args.test_path),
        title.profile_option, str(root),
        f"--gtest_filter={chapter_test_name(title, chapter)}",
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
    output = saves / slot_name(title, chapter)
    if output.is_symlink() or not output.is_file() or output.stat().st_size <= 1024:
        raise RuntimeError(f"C{chapter} did not write a regular chapter save")
    for earlier_chapter, before in earlier.items():
        if digest(saves / slot_name(title, earlier_chapter)) != before or not is_passed(title, saves, earlier_chapter):
            raise RuntimeError(f"C{chapter} changed an earlier chapter save or pass marker")
    validate_profile(title, root)
    (saves / pass_marker_name(title, chapter)).write_text(digest(output) + "\n")
    print(f"C{chapter}: passed, save and pass marker kept")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--title", choices=sorted(TITLES), default="mm7", help="Game whose chain to run")
    parser.add_argument("--data-path", type=Path, help="Local game data, MM6_DATA_PATH by default for MM6")
    parser.add_argument("--rules-path", type=Path, help="Private MM6 rules, MM6_RULES_PATH by default")
    parser.add_argument("--game-test", type=Path, default=REPO / "build/test/Bin/GameTest/OpenEnroth_GameTest")
    parser.add_argument("--test-path", type=Path, default=REPO / "test/Data")
    parser.add_argument("--through", type=int, default=0)
    parser.add_argument("--profile", type=Path, help="Resume a marked profile from a previous run")
    parser.add_argument("--remove-profile", action="store_true", help="Remove a marked profile without running chapters")
    args = parser.parse_args()

    title = TITLES[args.title]
    if not 0 <= args.through < title.chapters:
        parser.error(f"argument --through: invalid choice: {args.through} "
                     f"(choose from {', '.join(map(str, range(title.chapters)))})")
    if args.remove_profile:
        if args.profile is None:
            parser.error("--remove-profile requires --profile")
        return args
    if title.rules_env is None and args.rules_path is not None:
        parser.error("--rules-path is only for --title mm6")
    if args.data_path is None and title.data_env and os.environ.get(title.data_env):
        args.data_path = Path(os.environ[title.data_env])
    if args.data_path is None:
        parser.error("--data-path is required")
    if title.rules_env and args.rules_path is None:
        if not os.environ.get(title.rules_env):
            parser.error("--rules-path is required")
        args.rules_path = Path(os.environ[title.rules_env])
    return args


def main(args):
    title = TITLES[args.title]
    if args.remove_profile:
        root = validate_profile(title, args.profile)
        shutil.rmtree(root)
        print("Marked profile removed")
        return 0
    if not args.game_test.is_file() or not args.test_path.is_dir() or not args.data_path.is_dir() or \
            (title.rules_env and not args.rules_path.is_dir()):
        raise ValueError(f"GameTest binary, test path, or {title.paths} is missing")
    require_chapter_tests(title, args)
    if args.profile is None:
        root = Path(tempfile.mkdtemp(prefix=title.prefix))
        os.chmod(root, 0o700)
        (root / title.marker).write_text(title.marker_text)
    else:
        root = validate_profile(title, args.profile)
    print(f"Profile: {root}")
    start = passed_chapters(title, root)
    if start > args.through:
        print(f"C0-C{args.through}: already passed")
        return 0
    remove_unverified(title, root, start)
    for chapter in range(start, args.through + 1):
        run_chapter(title, args, root, chapter)
    if passed_chapters(title, root) <= args.through:
        raise RuntimeError(f"C0-C{args.through} do not all have pass markers")
    return 0


if __name__ == "__main__":
    arguments = parse_args()
    try:
        sys.exit(main(arguments))
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"{arguments.title.upper()} campaign chain: {error}", file=sys.stderr)
        sys.exit(1)
