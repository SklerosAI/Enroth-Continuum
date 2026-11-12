#!/usr/bin/env python3
"""Stage a title-checked local Atlas without replacing another campaign."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parent.parent
ATLAS_ICON = ROOT / "launcher/AtlasIcon.icns"
SOURCE_MANIFEST = ROOT / "thirdparty/repo-source/SOURCES.json"
IDENTITIES = {
    "MM6": ("org.sklerosai.enrothcontinuum.mm6", "MM6", "mm7"),
    "MM7": ("org.sklerosai.enrothcontinuum.mm7", "MM7", "mm6"),
    "MM8": ("org.sklerosai.enrothcontinuum.mm8", "MM8", "mm7"),
    # Upstream OpenEnroth plays MM7 only and has no --game option to probe.
    "OpenEnroth": ("OpenEnroth", "OpenEnroth", None),
}
HELPER_OPTIONS = {"MM6": "mm6_app", "MM7": "mm7_app", "MM8": "mm8_app", "OpenEnroth": "openenroth_app"}


def run(*args: str | Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run([str(arg) for arg in args], check=True, text=True,
                          capture_output=True)


def verify_helper(path: Path, title: str) -> None:
    expected_id, expected_executable, wrong_game = IDENTITIES[title]
    plist_path = path / "Contents/Info.plist"
    if not plist_path.is_file():
        raise ValueError(f"{title} helper is missing Info.plist: {path}")
    with plist_path.open("rb") as stream:
        info = plistlib.load(stream)
    if info.get("CFBundleIdentifier") != expected_id or info.get("CFBundleExecutable") != expected_executable:
        raise ValueError(f"{title} helper has the wrong bundle identity: {path}")

    binary = path / "Contents/MacOS" / expected_executable
    if not binary.is_file() or not os.access(binary, os.X_OK):
        raise ValueError(f"{title} helper executable is missing: {binary}")
    if "arm64" not in run("lipo", "-archs", binary).stdout.split():
        raise ValueError(f"{title} helper is not arm64: {binary}")
    if wrong_game is None:
        return

    # Bundle metadata alone is insufficient: prove the executable rejects
    # the other playable title before it can initialize data or a GUI.
    try:
        result = subprocess.run([str(binary), "--game", wrong_game, "--headless"],
                                text=True, capture_output=True, timeout=5)
    except subprocess.TimeoutExpired as exc:
        # A macOS GUI bundle can stay resident after its command-line main
        # returns, so its stdout never becomes observable by communicate().
        # Retain the title-lock check by requiring the exact compiled rejection
        # text in the helper, rather than treating the bundle lifecycle as a
        # false negative. Non-bundle/returning helpers still take the dynamic
        # path below.
        strings = subprocess.run(["strings", str(binary)], text=True, capture_output=True, check=True).stdout
        expected = f"This {title} app only accepts --game {title.lower()}."
        if expected not in strings:
            raise ValueError(f"{title} helper did not contain its title lock: {binary}") from exc
        return
    if result.returncode == 0 or f"only accepts --game {title.lower()}" not in result.stdout + result.stderr:
        raise ValueError(f"{title} helper did not prove its title lock: {binary}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def verify_atlas_icon() -> None:
    image = ATLAS_ICON.read_bytes()
    if len(image) < 8 or image[:4] != b"icns" or int.from_bytes(image[4:8], "big") != len(image):
        raise ValueError(f"The tracked Atlas icon is invalid: {ATLAS_ICON}")


def verify_mm7_dependency_notices(build_root: Path) -> Path:
    report_path = build_root / "build-report.json"
    report = json.loads(report_path.read_text())
    if report.get("architecture") != "arm64":
        raise ValueError("MM7 dependency report is not for arm64")
    manifest = json.loads(SOURCE_MANIFEST.read_text())
    expected_commits = {row["path"]: row["commit"] for row in manifest["sources"]}
    if report.get("source_commits") != expected_commits:
        raise ValueError("MM7 dependency report does not match tracked source pins")

    archives = report.get("archives_sha256")
    if not isinstance(archives, dict) or len(archives) != 9:
        raise ValueError("MM7 dependency report must list nine static archives")
    library_root = build_root / "prefix/lib"
    installed_archives = {path.name for path in library_root.glob("*.a") if not path.is_symlink()}
    if installed_archives != set(archives) or list(library_root.glob("*.dylib")):
        raise ValueError("MM7 dependency archive set differs from the report")
    for name, digest in archives.items():
        if Path(name).name != name or not name.endswith(".a"):
            raise ValueError(f"Invalid MM7 dependency archive name: {name}")
        archive = build_root / "prefix/lib" / name
        if archive.is_symlink() or not archive.is_file() or sha256(archive) != digest:
            raise ValueError(f"MM7 dependency archive is missing or changed: {name}")

    notices_root = build_root / "prefix/share/licenses/enroth-mm7-source"
    notices = report.get("notice_files_sha256")
    if not isinstance(notices, dict) or not notices:
        raise ValueError("MM7 dependency report has no source notices")
    actual_notices = {
        str(path.relative_to(notices_root))
        for path in notices_root.rglob("*") if path.is_file()
    }
    if actual_notices != set(notices) or any(path.is_symlink() for path in notices_root.rglob("*")):
        raise ValueError("MM7 dependency notice set differs from the report")
    for row in manifest["sources"]:
        required = f"{row['path']}/{row['primary_license_file']}"
        if required not in notices:
            raise ValueError(f"MM7 dependency report omits {required}")
    for name, digest in notices.items():
        relative = Path(name)
        if relative.is_absolute() or ".." in relative.parts or "." in relative.parts:
            raise ValueError(f"Invalid MM7 dependency notice path: {name}")
        notice = notices_root / relative
        if notice.is_symlink() or not notice.is_file() or sha256(notice) != digest:
            raise ValueError(f"MM7 dependency notice is missing or changed: {name}")
    return notices_root


def assemble(output: Path, helpers: dict[str, Path], mm7_deps_build_root: Path | None = None,
             build_infos: dict[str, Path] | None = None) -> None:
    sdk = Path(run("/usr/bin/xcrun", "--sdk", "macosx", "--show-sdk-path").stdout.strip())
    swiftc = Path(run("/usr/bin/xcrun", "--find", "swiftc").stdout.strip())
    if not sdk.is_dir() or not swiftc.is_file():
        raise ValueError("The installed Apple macOS SDK or Swift compiler is missing")
    if output.is_relative_to(Path("/Applications")):
        raise ValueError("Build into a staging location; install only after launch and save checks")
    for title, path in helpers.items():
        verify_helper(path, title)
    verify_atlas_icon()
    notice_source = verify_mm7_dependency_notices(mm7_deps_build_root) if mm7_deps_build_root else None

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".atlas-stage-", dir=output.parent) as temporary:
        temporary_root = Path(temporary)
        app = temporary_root / output.name
        contents = app / "Contents"
        for folder in ("MacOS", "Resources/Atlas", "Resources/Notices", "Resources/Helpers", "Helpers"):
            (contents / folder).mkdir(parents=True)

        cache = Path.home() / "Library/Caches/EnrothContinuum/SwiftModules"
        cache.mkdir(parents=True, exist_ok=True)
        run(swiftc, "-O", "-sdk", sdk, "-module-cache-path", cache,
            "-framework", "AppKit", "-framework", "WebKit", "-framework", "Carbon",
            ROOT / "launcher/AtlasApp.swift", ROOT / "launcher/MM7ProfilePath.swift",
            ROOT / "launcher/GameBuild.swift",
            "-o", contents / "MacOS/EnrothContinuum")
        shutil.copy2(ROOT / "launcher/Info.plist", contents / "Info.plist")
        shutil.copy2(ROOT / "launcher/Atlas/index.html", contents / "Resources/Atlas/index.html")
        shutil.copy2(ROOT / "tools/mm7_playtest_capture.sh", contents / "Resources/Atlas/mm7_playtest_capture.sh")
        for source in (ROOT / "LICENSE", ROOT / "CREDITS.md", ROOT / "project/UPSTREAM.md"):
            shutil.copy2(source, contents / "Resources/Notices" / source.name)
        if notice_source:
            shutil.copytree(notice_source, contents / "Resources/Notices/MM7-Media")
        shutil.copy2(ATLAS_ICON, contents / "Resources/AtlasIcon.icns")

        for title, source in (build_infos or {}).items():
            shutil.copy2(source, contents / "Resources/Helpers" / f"{title}-BUILD-INFO.txt")
        for title, source in helpers.items():
            destination = contents / "Helpers" / f"{title}.app"
            run("ditto", source, destination)
            verify_helper(destination, title)
            run("codesign", "--force", "--sign", "-", destination)
            run("codesign", "--verify", "--strict", destination)
        run("codesign", "--force", "--sign", "-", app)
        run("codesign", "--verify", "--strict", app)

        previous = temporary_root / "previous.app"
        if output.exists():
            os.rename(output, previous)
        try:
            os.rename(app, output)
        except BaseException:
            if previous.exists():
                os.rename(previous, output)
            raise
    print(output)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mm7-app", type=Path, help="Verified, title-locked MM7.app")
    parser.add_argument("--mm6-app", type=Path, help="Verified MM6.app, only after its play gate")
    parser.add_argument("--mm8-app", type=Path, help="Verified MM8.app, only after its play gate")
    parser.add_argument("--openenroth-app", type=Path,
                        help="Optional upstream OpenEnroth.app for playtesting; its sibling BUILD-INFO.txt is kept")
    parser.add_argument("--mm7-deps-build-root", type=Path,
                        help="Local MM7 media-library recipe output; verify and bundle its source notices")
    parser.add_argument("--reuse-from", type=Path, help="Existing Atlas with sibling helpers to retain")
    parser.add_argument("--output", type=Path, default=ROOT / "build/Atlas/Enroth Continuum.app")
    options = parser.parse_args()

    if options.reuse_from is not None and not options.reuse_from.is_dir():
        raise ValueError(f"Atlas to reuse does not exist: {options.reuse_from}")

    helpers = {}
    build_infos = {}
    for title, option in HELPER_OPTIONS.items():
        explicit = getattr(options, option)
        inherited = (options.reuse_from / "Contents/Helpers" / f"{title}.app"
                     if options.reuse_from else None)
        source = explicit if explicit is not None else inherited
        if source is not None and source.is_dir():
            helpers[title] = source.resolve()
            build_info = (source.parent / "BUILD-INFO.txt" if explicit is not None else
                          options.reuse_from / "Contents/Resources/Helpers" / f"{title}-BUILD-INFO.txt")
            if build_info.is_file():
                build_infos[title] = build_info.resolve()
        elif explicit is not None:
            raise ValueError(f"Explicit {title} helper does not exist: {explicit}")
    if "MM7" not in helpers:
        raise ValueError("A title-locked MM7.app is required; pass --mm7-app or --reuse-from")
    assemble(options.output.resolve(), helpers,
             options.mm7_deps_build_root.expanduser().resolve() if options.mm7_deps_build_root else None,
             build_infos)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Atlas assembly failed: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError):
            print(error.stdout or "", error.stderr or "", file=sys.stderr)
        sys.exit(1)
