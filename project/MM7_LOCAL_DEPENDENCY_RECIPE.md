# Repository-owned Mac MM7 media libraries

Status (2026-11-07): an original, opt-in source-build recipe is staged in
[`tools/build_mm7_macos_dependencies.py`](../tools/build_mm7_macos_dependencies.py).
It is connected to the opt-in CMake `EC_LOCAL_ONLY_CONFIGURE` lane and passed
a clean tracked-source build under network denial. It does not replace the
installed MM7 helper or the ordinary build's cached prebuilt archive.

The recipe checks the five commit pins in
[`thirdparty/repo-source/SOURCES.json`](../thirdparty/repo-source/SOURCES.json),
requires each original license file, and builds static zlib-ng, libpng,
OpenAL Soft, SDL3 and FFmpeg archives into a new local prefix. It sets an
arm64 macOS 11 deployment target, selects the Apple C/C++/Objective-C toolchain
with `xcrun`, clears inherited Conda compiler/linker flags, and restricts
package search away from common Homebrew/Conda/X11 prefixes. libpng is given
the exact locally built zlib archive and
headers. OpenAL Soft uses CoreAudio without optional external audio backends;
SDL3 omits libusb. FFmpeg disables automatic external-library detection, GPL,
version-3 and nonfree components and enables only the game-media families
listed in the recipe. Its generated `config.h` is checked for the three
license-mode flags before compilation. The recipe copies upstream license and
notice files beside the output and records archive/config hashes in
`build-report.json`. It rejects unexpected archives, dynamic libraries and
non-arm64 outputs.

Run on an Apple silicon Mac with CMake, Python 3 and the Apple command-line
toolchain already installed:

```sh
python3 tools/build_mm7_macos_dependencies.py \
  build/mm7-repo-deps-release \
  --build-type Release \
  --cmake /absolute/path/to/cmake \
  --jobs 4
```

The output directory must be empty. It is a local build product, not tracked
source. The recipe uses no download step. A native Release run completed all
five builds and produced exactly nine regular arm64 static archives in a 90 MiB
local output, with no dynamic libraries and 23 copied license/notice files.
libpng's CMake cache selected the just-built local zlib archive and headers;
FFmpeg's generated `config.h` reported GPL, version-3 and nonfree disabled. A
fresh isolated CMake package-find probe resolved ZLIB, PNG, SDL3, OpenAL Soft
and the five FFmpeg libraries from this prefix. A clean tracked-source extraction now links an MM7 helper against these
archives under network denial; a focused headless MM7 control passed. Visible
decode/audio/render behavior, a remote clone and binary distribution review
remain separate gates. The normal CMake path
still uses its cached prebuilt library archive.

## Opt-in CMake lane

The `EC_LOCAL_ONLY_CONFIGURE=ON` path accepts the recipe's `prefix/` through
`EC_LOCAL_DEPS_PREFIX`. It checks the report, pinned source revisions, all nine
archive hashes and arm64 slices, the FFmpeg configuration hash/license flags,
and every copied notice before setting exact CMake package paths. It also
points cpptrace's libdwarf FetchContent source override to the separately
tracked `libdwarf-lite/` snapshot. The normal build path is unchanged.

```sh
cmake -S . -B build/mm7-local-only \
  -DEC_LOCAL_ONLY_CONFIGURE=ON \
  -DEC_LOCAL_DEPS_PREFIX=/absolute/path/to/build/mm7-repo-deps-release/prefix \
  -DOE_USE_PREBUILT_DEPENDENCIES=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64
```

Add `-DEC_MM7_MAC_ICONV_ADAPTER=ON` for the opt-in Mac MM7 route. It uses an
original system-iconv adapter for the production UTF-8, Windows-1251 and ASCII
call sites and bypasses the unresolved ztd gitlink/nested fetches. A fresh
tracked-source configure, MM7 link and Atlas package passed under network
denial with this option. Without it, the gate still fails closed on ztd inputs.
Wrong build-type, absent-prefix and changed-archive-hash probes also failed
closed. Full legacy/CJK encoding parity, remote-clone repetition and graphical
play/save checks remain open.
