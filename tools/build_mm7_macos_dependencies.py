#!/usr/bin/env python3
"""Build the pinned, repository-owned MM7 media libraries for Apple silicon.

This is an Enroth Continuum recipe. It uses only source trees tracked in this
repository and the installed Apple toolchain. Original third-party licenses
and notices remain in their source trees and are copied beside the build.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys


REPO = Path(__file__).resolve().parents[1]
SOURCES = REPO / "thirdparty" / "repo-source"
EXPECTED_COMMITS = {
    "SDL3": "a96677bdf6b4acb84af4ec294e5f60a4e8cbbe03",
    "ffmpeg": "e3a61e91030696348b56361bdf80ea358aef4a19",
    "libpng": "f5e92d76973a7a53f517579bc95d61483bf108c0",
    "openal-soft": "dc7d7054a5b4f3bec1dc23a42fd616a0847af948",
    "zlib-ng": "12731092979c6d07f42da27da673a9f6c7b13586",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], *, cwd: Path | None = None, env: dict[str, str]) -> None:
    print("+", " ".join(command), flush=True)
    subprocess.run(command, cwd=cwd, env=env, check=True)


def cmake_project(
    cmake: str,
    cc: str,
    cxx: str,
    source: Path,
    build: Path,
    prefix: Path,
    build_type: str,
    jobs: int,
    extra: list[str],
    env: dict[str, str],
) -> None:
    common = [
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_INSTALL_PREFIX={prefix}",
        "-DCMAKE_OSX_ARCHITECTURES=arm64",
        "-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0",
        f"-DCMAKE_C_COMPILER={cc}",
        f"-DCMAKE_CXX_COMPILER={cxx}",
        f"-DCMAKE_OBJC_COMPILER={cc}",
        f"-DCMAKE_OBJCXX_COMPILER={cxx}",
        f"-DCMAKE_ASM_COMPILER={cc}",
        f"-DCMAKE_AR={env['AR']}",
        f"-DCMAKE_RANLIB={env['RANLIB']}",
        f"-DCMAKE_PREFIX_PATH={prefix}",
        "-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF",
        "-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF",
        "-DCMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH=OFF",
        "-DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local;/usr/X11R6;/opt/anaconda3",
        "-DFETCHCONTENT_FULLY_DISCONNECTED=ON",
    ]
    run([cmake, "-S", str(source), "-B", str(build), *common, *extra], env=env)
    run([cmake, "--build", str(build), "--parallel", str(jobs)], env=env)
    run([cmake, "--install", str(build)], env=env)


def ffmpeg_project(source: Path, build: Path, prefix: Path, jobs: int,
                   cc: str, cxx: str, env: dict[str, str]) -> None:
    build.mkdir(parents=True, exist_ok=True)
    configure = [
        str(source / "configure"),
        f"--prefix={prefix}",
        "--arch=arm64",
        "--target-os=darwin",
        f"--cc={cc}",
        f"--cxx={cxx}",
        f"--ar={env['AR']}",
        f"--ranlib={env['RANLIB']}",
        "--enable-static",
        "--disable-shared",
        "--disable-autodetect",
        "--disable-gpl",
        "--disable-version3",
        "--disable-nonfree",
        "--disable-everything",
        "--disable-avdevice",
        "--disable-avfilter",
        "--disable-postproc",
        "--disable-programs",
        "--disable-doc",
        "--disable-network",
        "--disable-iconv",
        "--enable-avcodec",
        "--enable-avformat",
        "--enable-avutil",
        "--enable-swresample",
        "--enable-swscale",
        "--enable-decoder=mp3*",
        "--enable-decoder=adpcm*",
        "--enable-decoder=pcm*",
        "--enable-decoder=bink",
        "--enable-decoder=binkaudio_dct",
        "--enable-decoder=binkaudio_rdft",
        "--enable-decoder=smackaud",
        "--enable-decoder=smacker",
        "--enable-demuxer=mp3",
        "--enable-demuxer=bink",
        "--enable-demuxer=binka",
        "--enable-demuxer=smacker",
        "--enable-demuxer=pcm*",
        "--enable-demuxer=wav",
        "--enable-protocol=file",
        "--extra-cflags=-arch arm64 -mmacosx-version-min=11.0",
        "--extra-ldflags=-arch arm64 -mmacosx-version-min=11.0",
    ]
    run(configure, cwd=build, env=env)
    config = (build / "config.h").read_text()
    for setting in ("CONFIG_GPL", "CONFIG_VERSION3", "CONFIG_NONFREE"):
        if f"#define {setting} 0" not in config:
            raise RuntimeError(f"FFmpeg {setting} was not disabled")
    run(["/usr/bin/make", "-C", str(build), f"-j{jobs}", "install"], env=env)


def copy_notices(prefix: Path) -> dict[str, str]:
    notices: dict[str, str] = {}
    destination = prefix / "share" / "licenses" / "enroth-mm7-source"
    for family in EXPECTED_COMMITS:
        source = SOURCES / family
        for file in source.rglob("*"):
            if not file.is_file():
                continue
            name = file.name.upper()
            if not name.startswith(("LICENSE", "COPYING", "NOTICE")):
                continue
            relative = file.relative_to(SOURCES)
            target = destination / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(file, target)
            notices[str(relative)] = sha256(target)
    return notices


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="new, empty build root (outside the source trees)")
    parser.add_argument("--build-type", choices=("Debug", "Release", "RelWithDebInfo"), default="Release")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--cmake", default=shutil.which("cmake"))
    args = parser.parse_args()
    if platform.system() != "Darwin" or platform.machine() != "arm64":
        parser.error("this recipe currently supports native macOS arm64 only")
    if not args.cmake or not Path(args.cmake).is_file():
        parser.error("pass --cmake with an installed CMake executable")
    if args.jobs < 1:
        parser.error("--jobs must be positive")

    manifest = json.loads((SOURCES / "SOURCES.json").read_text())
    pinned = {row["path"]: row["commit"] for row in manifest["sources"]}
    if pinned != EXPECTED_COMMITS:
        parser.error("repository-owned source pins differ from this reviewed recipe")
    for family in EXPECTED_COMMITS:
        if not (SOURCES / family).is_dir():
            parser.error(f"missing repository-owned source: {family}")
        license_path = next(row["primary_license_file"] for row in manifest["sources"] if row["path"] == family)
        if not (SOURCES / family / license_path).is_file():
            parser.error(f"missing upstream license: {family}/{license_path}")

    output = args.output.expanduser().resolve()
    if output == SOURCES or SOURCES in output.parents:
        parser.error("build output must be outside thirdparty/repo-source")
    if output.exists() and any(output.iterdir()):
        parser.error("output directory must be empty; use a new path for each build")
    output.mkdir(parents=True, exist_ok=True)
    prefix = output / "prefix"
    prefix.mkdir()
    build = output / "build"
    build.mkdir()
    cc = subprocess.check_output(["/usr/bin/xcrun", "--find", "clang"], text=True).strip()
    cxx = subprocess.check_output(["/usr/bin/xcrun", "--find", "clang++"], text=True).strip()
    ar = subprocess.check_output(["/usr/bin/xcrun", "--find", "ar"], text=True).strip()
    ranlib = subprocess.check_output(["/usr/bin/xcrun", "--find", "ranlib"], text=True).strip()
    env = os.environ.copy()
    for inherited in ("CC", "CXX", "OBJC", "OBJCXX", "AR", "RANLIB", "AS", "LD",
                      "CFLAGS", "CXXFLAGS", "OBJCFLAGS", "OBJCXXFLAGS", "CPPFLAGS", "LDFLAGS",
                      "CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH", "OBJC_INCLUDE_PATH",
                      "LIBRARY_PATH", "DYLD_LIBRARY_PATH", "CMAKE_FRAMEWORK_PATH"):
        env.pop(inherited, None)
    env.update({
        "PATH": "/usr/bin:/bin:/usr/sbin:/sbin",
        "CC": cc,
        "CXX": cxx,
        "OBJC": cc,
        "OBJCXX": cxx,
        "AR": ar,
        "RANLIB": ranlib,
        "MACOSX_DEPLOYMENT_TARGET": "11.0",
        "PKG_CONFIG_PATH": str(prefix / "lib" / "pkgconfig"),
        "PKG_CONFIG_LIBDIR": str(prefix / "lib" / "pkgconfig"),
        "CMAKE_PREFIX_PATH": str(prefix),
    })

    cmake_project(args.cmake, cc, cxx, SOURCES / "zlib-ng", build / "zlib-ng", prefix,
                  args.build_type, args.jobs,
                  ["-DZLIB_COMPAT=ON", "-DBUILD_SHARED_LIBS=OFF", "-DBUILD_TESTING=OFF",
                   "-DCMAKE_DEBUG_POSTFIX=d"], env)
    zlib_library = prefix / "lib" / ("libzd.a" if args.build_type == "Debug" else "libz.a")
    if not zlib_library.is_file():
        raise RuntimeError(f"zlib-ng did not install expected static archive: {zlib_library}")
    cmake_project(args.cmake, cc, cxx, SOURCES / "libpng", build / "libpng", prefix,
                  args.build_type, args.jobs,
                  ["-DPNG_SHARED=OFF", "-DPNG_STATIC=ON", "-DPNG_FRAMEWORK=OFF",
                   "-DPNG_TESTS=OFF", "-DPNG_TOOLS=OFF", "-DPNG_EXECUTABLES=OFF",
                   f"-DZLIB_ROOT={prefix}", f"-DZLIB_LIBRARY={zlib_library}",
                   f"-DZLIB_INCLUDE_DIR={prefix / 'include'}", "-DZLIB_USE_STATIC_LIBS=ON"], env)
    cmake_project(args.cmake, cc, cxx, SOURCES / "openal-soft", build / "openal-soft", prefix,
                  args.build_type, args.jobs,
                  ["-DLIBTYPE=STATIC", "-DALSOFT_DLOPEN=OFF", "-DALSOFT_UTILS=OFF",
                   "-DALSOFT_EXAMPLES=OFF", "-DALSOFT_TESTS=OFF",
                   "-DALSOFT_UPDATE_BUILD_VERSION=OFF", "-DALSOFT_REQUIRE_COREAUDIO=ON",
                   "-DALSOFT_BACKEND_PIPEWIRE=OFF", "-DALSOFT_BACKEND_PULSEAUDIO=OFF",
                   "-DALSOFT_BACKEND_JACK=OFF", "-DALSOFT_BACKEND_PORTAUDIO=OFF",
                   "-DALSOFT_BACKEND_SDL3=OFF", "-DALSOFT_BACKEND_SDL2=OFF"], env)
    cmake_project(args.cmake, cc, cxx, SOURCES / "SDL3", build / "SDL3", prefix,
                  args.build_type, args.jobs,
                  ["-DSDL_STATIC=ON", "-DSDL_SHARED=OFF", "-DSDL_TEST_LIBRARY=OFF",
                   "-DSDL_TESTS=OFF", "-DSDL_EXAMPLES=OFF", "-DSDL_HIDAPI_LIBUSB=OFF",
                   "-DSDL_DEPS_SHARED=OFF"], env)
    ffmpeg_project(SOURCES / "ffmpeg", build / "ffmpeg", prefix, args.jobs, cc, cxx, env)

    expected = ("libSDL3.a", "libopenal.a", "libavcodec.a", "libavformat.a",
                "libavutil.a", "libswscale.a", "libswresample.a")
    archives = {name: sha256(prefix / "lib" / name) for name in expected}
    archives[zlib_library.name] = sha256(zlib_library)
    png_library = prefix / "lib" / ("libpng16d.a" if args.build_type == "Debug" else "libpng16.a")
    archives[png_library.name] = sha256(png_library)
    actual_archives = {p.name for p in (prefix / "lib").glob("*.a") if not p.is_symlink()}
    if actual_archives != set(archives):
        raise RuntimeError(f"unexpected static archive set: {sorted(actual_archives)}")
    if list((prefix / "lib").glob("*.dylib")):
        raise RuntimeError("a shared library was installed instead of a static-only prefix")
    for name in archives:
        info = subprocess.check_output(["/usr/bin/lipo", "-info", str(prefix / "lib" / name)],
                                       text=True).strip()
        if not info.endswith("architecture: arm64"):
            raise RuntimeError(f"{name} is not arm64: {info}")
    notices = copy_notices(prefix)
    report = {
        "source_commits": EXPECTED_COMMITS,
        "build_type": args.build_type,
        "architecture": "arm64",
        "deployment_target": "11.0",
        "compiler_paths": {"cc": cc, "cxx": cxx, "ar": ar, "ranlib": ranlib},
        "ffmpeg_config_h_sha256": sha256(build / "ffmpeg" / "config.h"),
        "archives_sha256": archives,
        "notice_files_sha256": notices,
    }
    (output / "build-report.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(f"Built {len(archives)} static archives from repository-owned sources in {prefix}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, subprocess.CalledProcessError, RuntimeError) as error:
        print(f"MM7 local dependency build failed: {error}", file=sys.stderr)
        sys.exit(1)
