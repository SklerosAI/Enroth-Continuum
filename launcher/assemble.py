#!/usr/bin/env python3
"""Stage a title-checked local Atlas without replacing another campaign."""

import argparse
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parent.parent
SDK = Path("/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk")
IDENTITIES = {
    "MM6": ("org.sklerosai.enrothcontinuum.mm6", "MM6", "mm7"),
    "MM7": ("org.sklerosai.enrothcontinuum.mm7", "MM7", "mm6"),
    "MM8": ("org.sklerosai.enrothcontinuum.mm8", "MM8", "mm7"),
}


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

    # Bundle metadata alone is insufficient: prove the executable rejects
    # the other playable title before it can initialize data or a GUI.
    try:
        result = subprocess.run([str(binary), "--game", wrong_game, "--headless"],
                                text=True, capture_output=True, timeout=5)
    except subprocess.TimeoutExpired as exc:
        raise ValueError(f"{title} helper did not reject the foreign title") from exc
    if result.returncode == 0 or f"only accepts --game {title.lower()}" not in result.stdout + result.stderr:
        raise ValueError(f"{title} helper did not prove its title lock: {binary}")


def render_icon(iconset: Path, destination: Path) -> None:
    iconset.mkdir()
    for size in (16, 32, 128, 256, 512):
        for multiplier, suffix in ((1, ""), (2, "@2x")):
            pixel_size = size * multiplier
            run("rsvg-convert", "-w", str(pixel_size), "-h", str(pixel_size),
                ROOT / "launcher/AtlasIcon.svg", "-o",
                iconset / f"icon_{size}x{size}{suffix}.png")
    run(sys.executable, ROOT / "launcher/make_icon.py", iconset, destination)


def assemble(output: Path, helpers: dict[str, Path]) -> None:
    if not SDK.is_dir():
        raise ValueError(f"The tested macOS SDK is missing: {SDK}")
    if output.is_relative_to(Path("/Applications")):
        raise ValueError("Build into a staging location; install only after launch and save checks")
    for title, path in helpers.items():
        verify_helper(path, title)

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".atlas-stage-", dir=output.parent) as temporary:
        temporary_root = Path(temporary)
        app = temporary_root / output.name
        contents = app / "Contents"
        for folder in ("MacOS", "Resources/Atlas", "Resources/Notices", "Helpers"):
            (contents / folder).mkdir(parents=True)

        cache = Path.home() / "Library/Caches/EnrothContinuum/SwiftModules"
        cache.mkdir(parents=True, exist_ok=True)
        run("swiftc", "-O", "-sdk", SDK, "-module-cache-path", cache,
            "-framework", "AppKit", "-framework", "WebKit",
            ROOT / "launcher/AtlasApp.swift", "-o", contents / "MacOS/EnrothContinuum")
        shutil.copy2(ROOT / "launcher/Info.plist", contents / "Info.plist")
        shutil.copy2(ROOT / "launcher/Atlas/index.html", contents / "Resources/Atlas/index.html")
        for source in (ROOT / "LICENSE", ROOT / "CREDITS.md", ROOT / "project/UPSTREAM.md"):
            shutil.copy2(source, contents / "Resources/Notices" / source.name)
        render_icon(temporary_root / "iconset", contents / "Resources/AtlasIcon.icns")

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
    parser.add_argument("--reuse-from", type=Path, help="Existing Atlas with sibling helpers to retain")
    parser.add_argument("--output", type=Path, default=ROOT / "build/Atlas/Enroth Continuum.app")
    options = parser.parse_args()

    if options.reuse_from is not None and not options.reuse_from.is_dir():
        raise ValueError(f"Atlas to reuse does not exist: {options.reuse_from}")

    helpers = {}
    for title in ("MM6", "MM7", "MM8"):
        explicit = getattr(options, f"{title.lower()}_app")
        inherited = (options.reuse_from / "Contents/Helpers" / f"{title}.app"
                     if options.reuse_from else None)
        source = explicit if explicit is not None else inherited
        if source is not None and source.is_dir():
            helpers[title] = source.resolve()
        elif explicit is not None:
            raise ValueError(f"Explicit {title} helper does not exist: {explicit}")
    if "MM7" not in helpers:
        raise ValueError("A title-locked MM7.app is required; pass --mm7-app or --reuse-from")
    assemble(options.output.resolve(), helpers)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Atlas assembly failed: {error}", file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError):
            print(error.stdout or "", error.stderr or "", file=sys.stderr)
        sys.exit(1)
