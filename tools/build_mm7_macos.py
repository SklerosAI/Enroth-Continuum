#!/usr/bin/env python3
"""Build and stage the repository-owned Mac MM7 helper without downloads.

The result is a staged Atlas bundle. This command never installs an app or
copies game data or saves. Use a new absolute output directory for each run.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys


REPO = Path(__file__).resolve().parents[1]
ARCHIVES = {
    "libSDL3.a", "libopenal.a", "libavcodec.a", "libavformat.a",
    "libavutil.a", "libswscale.a", "libswresample.a", "libz.a",
    "libpng16.a",
}
REQUIRED_SOURCE = (
    "tools/build_mm7_macos_dependencies.py",
    "thirdparty/repo-source/SOURCES.json",
    "thirdparty/repo-source/libdwarf-lite/CMakeLists.txt",
    "src/Utility/String/EncodingMac.cpp",
    "launcher/assemble.py",
    "launcher/AtlasIcon.icns",
)
INHERITED_BUILD_ENV = (
    "CC", "CXX", "OBJC", "OBJCXX", "CFLAGS", "CXXFLAGS", "CPPFLAGS",
    "LDFLAGS", "SDKROOT", "MACOSX_DEPLOYMENT_TARGET", "CMAKE_GENERATOR",
    "CMAKE_PREFIX_PATH", "PKG_CONFIG_PATH", "PKG_CONFIG_LIBDIR",
)


def tool_path(value: str | None, name: str) -> Path:
    candidate = value or shutil.which(name)
    if not candidate:
        raise ValueError(f"{name} is required; provide --{name} with its executable path")
    path = Path(candidate).expanduser().resolve()
    if not path.is_file() or not os.access(path, os.X_OK):
        raise ValueError(f"{name} is not executable: {path}")
    return path


def run(command: list[str], *, log: Path, environment: dict[str, str]) -> str:
    with log.open("w", encoding="utf-8") as stream:
        result = subprocess.run(command, cwd=REPO, env=environment,
                                stdout=stream, stderr=subprocess.STDOUT,
                                check=False)
    output = log.read_text(encoding="utf-8", errors="replace")
    if result.returncode:
        tail = "\n".join(output.splitlines()[-25:])
        raise RuntimeError(f"Command failed ({result.returncode}); see {log}\n{tail}")
    return output


def check_output_root(path: Path) -> Path:
    if not path.is_absolute():
        raise ValueError("The output root must be an absolute path")
    root = path.expanduser().resolve()
    if (root.is_relative_to(Path("/Applications")) or
            root.is_relative_to(Path("/System/Applications"))):
        raise ValueError("Build and stage outside Applications")
    if root.is_relative_to(REPO) or REPO.is_relative_to(root):
        raise ValueError("The output root must be separate from the source checkout")
    private_root = Path.home() / "Library/Application Support/Enroth Continuum"
    if root.is_relative_to(private_root):
        raise ValueError("The output root must be separate from private game data and saves")
    if root.exists():
        raise ValueError(f"The output root already exists; choose a new path: {root}")
    return root


def verify_link_inputs(build: Path, prefix: Path, report: dict, ninja: Path,
                       environment: dict[str, str], log: Path) -> None:
    archives = report.get("archives_sha256")
    if not isinstance(archives, dict) or set(archives) != ARCHIVES:
        raise RuntimeError("The dependency recipe did not produce the expected nine archives")
    commands = run([str(ninja), "-C", str(build), "-t", "commands", "MM7"],
                   log=log, environment=environment)
    for name in ARCHIVES:
        if str(prefix / "lib" / name) not in commands:
            raise RuntimeError(f"MM7 link does not use the local {name}")
    if "deps_r8_master" in commands or "ztd_text" in commands:
        raise RuntimeError("MM7 build still references an upstream archive or ztd.text")


def verify_system_links(binary: Path, environment: dict[str, str], log: Path) -> None:
    if not binary.is_file():
        raise RuntimeError(f"MM7 helper is missing: {binary}")
    architectures = run(["/usr/bin/lipo", "-archs", str(binary)],
                        log=log, environment=environment).split()
    if architectures != ["arm64"]:
        raise RuntimeError(f"MM7 helper has unexpected architectures: {architectures}")
    linked = run(["/usr/bin/otool", "-L", str(binary)],
                 log=log, environment=environment).splitlines()[1:]
    for line in linked:
        library = line.strip().split(" ", 1)[0]
        if library and not library.startswith(("/usr/lib/", "/System/Library/")):
            raise RuntimeError(f"MM7 helper links a non-system dynamic library: {library}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output_root", type=Path,
                        help="new absolute build/staging directory outside Applications")
    parser.add_argument("--cmake", help="CMake executable; defaults to cmake in PATH")
    parser.add_argument("--ninja", help="Ninja executable; defaults to ninja in PATH")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()

    if sys.platform != "darwin" or platform.machine() != "arm64":
        raise ValueError("This command requires native Apple silicon macOS")
    if args.jobs < 1:
        raise ValueError("--jobs must be positive")
    output = check_output_root(args.output_root)
    cmake = tool_path(args.cmake, "cmake")
    ninja = tool_path(args.ninja, "ninja")
    for relative in REQUIRED_SOURCE:
        if not (REPO / relative).is_file():
            raise ValueError(f"Required tracked source is missing: {relative}")
    for system_tool in ("/usr/bin/xcrun", "/usr/bin/lipo", "/usr/bin/otool"):
        if not Path(system_tool).is_file():
            raise ValueError(f"Required Apple tool is missing: {system_tool}")

    environment = os.environ.copy()
    for name in INHERITED_BUILD_ENV:
        environment.pop(name, None)
    cc = subprocess.check_output(["/usr/bin/xcrun", "--find", "clang"],
                                 text=True, env=environment).strip()
    cxx = subprocess.check_output(["/usr/bin/xcrun", "--find", "clang++"],
                                  text=True, env=environment).strip()
    sdk = subprocess.check_output(["/usr/bin/xcrun", "--sdk", "macosx",
                                   "--show-sdk-path"], text=True,
                                  env=environment).strip()
    if not Path(sdk).is_dir():
        raise ValueError("The installed Apple macOS SDK is missing")
    # libpng generates a C file with a direct compiler invocation. Give that
    # child the same SDK as CMake instead of inheriting an arbitrary SDKROOT.
    environment["SDKROOT"] = sdk

    output.mkdir(parents=True)
    logs = output / "logs"
    logs.mkdir()
    dependencies = output / "dependencies"
    build = output / "engine"
    atlas = output / "Atlas" / "Enroth Continuum.app"

    print("Building repository-owned MM7 media libraries...", flush=True)
    run([sys.executable, str(REPO / "tools/build_mm7_macos_dependencies.py"),
         str(dependencies), "--build-type", "Release", "--cmake", str(cmake),
         "--jobs", str(args.jobs)], log=logs / "dependencies.log",
        environment=environment)
    report = json.loads((dependencies / "build-report.json").read_text())
    prefix = (dependencies / "prefix").resolve()

    print("Configuring the local-only MM7 engine...", flush=True)
    run([str(cmake), "-S", str(REPO), "-B", str(build), "-G", "Ninja",
         f"-DCMAKE_MAKE_PROGRAM={ninja}", "-DCMAKE_BUILD_TYPE=Release",
         "-DCMAKE_OSX_ARCHITECTURES=arm64",
         f"-DCMAKE_OSX_SYSROOT={sdk}",
         "-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0",
         f"-DCMAKE_C_COMPILER={cc}", f"-DCMAKE_CXX_COMPILER={cxx}",
         "-DEC_LOCAL_ONLY_CONFIGURE=ON", "-DEC_MM7_MAC_ICONV_ADAPTER=ON",
         f"-DEC_LOCAL_DEPS_PREFIX={prefix}",
         "-DOE_USE_PREBUILT_DEPENDENCIES=OFF", "-DOE_BUILD_TESTS=OFF",
         "-DOE_BUILD_TOOLS=OFF", "-DOE_CHECK_STYLE=OFF",
         "-DOE_CHECK_LUA_STYLE=OFF", "-DOE_CHECK_TIDY=OFF"],
        log=logs / "configure.log", environment=environment)
    print("Compiling the MM7 helper...", flush=True)
    run([str(cmake), "--build", str(build), "--target", "MM7",
         "--parallel", str(args.jobs)], log=logs / "build.log",
        environment=environment)
    verify_link_inputs(build, prefix, report, ninja, environment,
                       logs / "link-inputs.log")
    helper = build / "src/Bin/OpenEnroth/MM7.app"
    verify_system_links(helper / "Contents/MacOS/MM7", environment,
                        logs / "linked-libraries.log")

    print("Staging Atlas with MM7 notices...", flush=True)
    run([sys.executable, str(REPO / "launcher/assemble.py"),
         "--mm7-app", str(helper), "--mm7-deps-build-root", str(dependencies),
         "--output", str(atlas)], log=logs / "atlas.log",
        environment=environment)
    if not (atlas / "Contents/Helpers/MM7.app/Contents/MacOS/MM7").is_file():
        raise RuntimeError("Atlas assembly did not stage the MM7 helper")
    print(f"Staged Atlas: {atlas}")
    print(f"Build logs: {logs}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"MM7 local build failed: {exc}", file=sys.stderr)
        sys.exit(1)
