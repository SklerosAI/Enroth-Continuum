# MM7 arm64 build-input inventory and migration packet

Status: **read-only local inventory**, 2026-11-06, Enroth Continuum source `9ae53a35a2e2af9dcff691386bb75c34e578c97a`. This records the existing Debug arm64 build and checked-out source. It is not a clean-clone or offline-build result. No dependency, game data, save or installed app was copied or changed.

Migration update, 2026-11-07: `thirdparty/generator`, `fast_float`,
`magic_enum`, `mio`, `small_vector`, `cmrc` and `cli11` are now ordinary
repository-owned source snapshots at their former gitlink revisions. Their
original files matched the pinned Git blobs (11 plus 112 plus 379), and full
original licenses and Enroth provenance notes are retained.
Network-disconnected standalone source builds/runs passed.
The table below remains the historical 23-gitlink inventory; the current
repository has **16 gitlinks**. The integration Mac MM7 target links with the
seven tracked source snapshots, using its existing cached prebuilt libraries.
The remaining gitlinks, nested fetches and prebuilt-library families still
block a clean offline MM7 build.

## Evidence boundary

The title-locked `MM7.app` is declared in [`src/Bin/OpenEnroth/CMakeLists.txt`](../src/Bin/OpenEnroth/CMakeLists.txt) and links this fork's `application` and library targets. Atlas stages an explicitly supplied helper; it is not an OpenEnroth release download. The inspected local cache is `build/CMakeCache.txt`: `Debug`, `arm64`, Ninja, `OE_USE_PREBUILT_DEPENDENCIES=ON`, `OE_BUILD_TESTS=ON`, `OE_BUILD_TOOLS=ON`. The exact `MM7.app/Contents/MacOS/MM7` rule in that build's `build.ninja` supplies the static-library inventory below. `otool -L` on the built MM7 executable lists Apple system libraries/frameworks and libc++, with no third-party dynamic library. Static libraries are build inputs incorporated into the executable, not separate app runtime files. Original MM7 data and local saves remain external and private.

Three network boundaries are distinct:

1. [`CMakeModules/Git.cmake`](../CMakeModules/Git.cmake) runs `git submodule update --init --recursive` unconditionally in the normal configure path when Git is found. This can contact the remaining 16 Git origins in [`.gitmodules`](../.gitmodules), even if the requested target is only MM7. A failed update is not checked there. The opt-in local-only configure path skips that update.
2. [`CMakeModules/Dependencies.cmake`](../CMakeModules/Dependencies.cmake) downloads `https://github.com/OpenEnroth/OpenEnroth_Dependencies/releases/download/deps_r8_master/darwin_Debug_arm64.zip` if the marker ZIP is absent. The code does not specify an expected content hash; an existing ZIP skips download. `OE_USE_PREBUILT_DEPENDENCIES=OFF` instead requires ZLIB, FFmpeg, SDL3, OpenAL Soft and PNG to be found elsewhere and does not itself make the build repo-contained.
3. The Mac [`thirdparty/CMakeLists.txt`](../thirdparty/CMakeLists.txt) includes cpptrace and ztd.text. cpptrace can fetch libdwarf-lite; ztd.text's own CMake fetches six projects by floating `main` branch. The seven cached `_deps/*-src` directories below show what this local configure resolved. Additional conditional ztd benchmark/docs fetches are present in its source but were not active in this build.

## Pinned Git submodules

Each row was a gitlink recorded by the original inventory snapshot, with its origin then in `.gitmodules`. The SHA is the full former gitlink commit; the size is the checked-out tree at inventory time (`du -sk`, rounded MiB), **not** an expected Git pack size. The seven rows named in the migration update are now repo-owned source at those same revisions. License labels are readings of the checked-out file named after the semicolon; keep each full file and attribution when migrating. “App configure” means this Mac CMake path adds or includes the source even when the MM7 link itself uses only headers or a subset of its targets. `cpplint`, GoogleTest and `gtest_parallel` are style/test inputs, not required to link a production MM7-only target, but the current normal configure still requests them.

| Path under `thirdparty/` | Origin in `.gitmodules` | Gitlink SHA | Local MiB | License evidence; role |
| --- | --- | --- | ---: | --- |
| `cli11` | `CLIUtils/CLI11` | `95324820650117a2668ed63b095007068fd0c475` | 1.9 | BSD-style; `LICENSE`; app configure/CLI |
| `cmrc` | `vector-of-bool/cmrc` | `952ffddba731fc110bd50409e8d2b8a06abbd237` | 2.6 | MIT; `LICENSE.txt`; embedded resources |
| `cpplint` | `cpplint/cpplint` | `45102d1497f3688c7c1eceffb7b87e1fdcce49f3` | 3.1 | BSD-style; `LICENSE`; optional style target |
| `cpptrace` | `jeremy-rifkin/cpptrace` | `3db8da80111171c219ab5839905771386bee06b3` | 1.8 | MIT; `LICENSE`; Mac stack traces, plus libdwarf fetch |
| `fast_float` | `fastfloat/fast_float` | `1ea4f27b2aeee2859a1354a3c24cff52a116cad1` | 0.4 | MIT or Apache-2.0; `LICENSE-MIT`, `LICENSE-APACHE`; app configure |
| `fmt` | `fmtlib/fmt` | `e3ddede6c4ee818825c4e5a6dfa1d384860c27d9` | 3.2 | MIT-style; `LICENSE`; linked app archive |
| `generator` | `TartanLlama/generator` | `2a912502de4f97dcba4f95c958ee0ddf7bc22cf5` | 0.1 | CC0-1.0; `COPYING`; event coroutine target |
| `glm` | `g-truc/glm` | `0af55ccecd98d4e5a8d1fad7de25ba429d60e863` | 24.1 | MIT; `copying.txt`; linked app archive |
| `googletest` | `google/googletest` | `3026483ae575e2de942db5e760cf95e973308dd5` | 4.3 | BSD-style; `LICENSE`; tests when `OE_BUILD_TESTS=ON` |
| `gtest_parallel` | `google/gtest-parallel` | `96f4f904922f9bf66689e749c40f314845baaac8` | 0.1 | Apache-2.0; `LICENSE`; test runner scripts |
| `imgui/imgui` | `ocornut/imgui` | `b48d1afbe8ee8b238e2961dc363a949dd7304e23` | 8.9 | MIT; `LICENSE.txt`; linked through tracked ImGui wrapper |
| `inifile_cpp` | `Rookfighter/inifile-cpp` | `7bb1ec3534768e0d1fd9893d01027468b72af5ec` | 4.8 | MIT; `LICENSE.txt`; app configure/header target |
| `lambda2` | `boostorg/lambda2` | `dddfec608515f5a4f51f86d9b8efc01c7ad816cc` | 0.2 | BSL-1.0; `doc/lambda2/copyright.adoc`; utility target |
| `luajit` | `LuaJIT/LuaJIT` | `97813fb924edf822455f91a5fbbdfdb349e5984f` | 4.7 | MIT-style; `COPYRIGHT`; linked app archive |
| `luajit-cmake` | `zhaozg/luajit-cmake` | `64462217e05164b46ca44c10ed1d12918eee8fa5` | 0.2 | MIT; `LICENSE`; LuaJIT build recipe |
| `magic_enum` | `Neargye/magic_enum` | `d01a4f90a63444e910e66c796b93df25abffcfe0` | 0.9 | MIT; `LICENSE`; app configure/header target |
| `mio` | `mandreyel/mio` | `8b6b7d878c89e81614d05edca7936de41ccdd2da` | 0.2 | MIT; `LICENSE`; memory-mapped files |
| `nlohmann_json` | `nlohmann/json` | `55f93686c01528224f448c19128836e7df245f72` | 18.6 | MIT; `LICENSE.MIT`; JSON target |
| `small_vector` | `gharveymn/small_vector` | `e8e63fe12ef578528ca5e954bc91321967e76393` | 1.2 | MIT; `LICENSE`; app configure |
| `sol2` | `ThePhD/sol2` | `336f76c3772c84e347a7f4ffb246d8c0e2d5f426` | 22.2 | MIT; `LICENSE.txt`; Lua binding target |
| `spdlog` | `gabime/spdlog` | `79524ddd08a4ec981b7fea76afd08ee05f83755d` | 1.6 | MIT; `LICENSE`; linked app archive |
| `tcpp` | `bnoazx005/tcpp` | `8041d013d3ad97c95c1e857f0bd3f8008becb72e` | 4.8 | Apache-2.0; `LICENSE`; preprocessor target |
| `ztd_text` | `captainurist/ztd_text` | `c17bbb1e0bbd953f65a1846e8df11939f83f2169` | 16.1 | **Review before copying:** `LICENSE.md` combines an Apache-2.0 body with “Non-Commerical License” and commercial-license provisions; `NOTICE`; encoding target |

The 23 checked-out trees total about **126 MiB**. `thirdparty/glad` and the ImGui integration wrapper are already tracked ordinary files, not two more gitlinks. The individual origin URLs are the `https://github.com/<origin>` values in `.gitmodules`. The upstream Git remote is not used for the MM7 target; the configure-time release URL is a separate dependency.

## Prebuilt arm64 archive actually used

The local `darwin_Debug_arm64.zip` is **21,714,289 bytes** (SHA-256 `1f109947f8d3fa9e20194ff9109f1f7e1a3bd12ed045cda33818c2fd09b90329`); its containing extracted directory is about **102 MiB including the ZIP**. This hash identifies this Mac's cached file, not an upstream expected digest. The MM7 link rule uses all nine archives below from it. Headers/configs report SDL3 3.2.22, OpenAL Soft 1.24.3, libpng 1.6.44 and zlib-ng compatibility header `1.3.1.zlib-ng`; FFmpeg headers report library version series `avcodec 61.3.100`, `avformat 61.1.100`, `avutil 59.8.100`, `swscale 8.1.100`, `swresample 5.1.100`. Exact source revisions, build options and complete license/notice bundle are **not present in the local ZIP**. `include/SDL3/SDL_copying.h` alone is not a complete archive provenance record; FFmpeg's effective license depends on its build configuration. Recreate these from pinned source/recipes and review them before any repository-owned source snapshot or redistribution.

| Archive in `lib/` | Bytes | Local MiB | Source family / evidence |
| --- | ---: | ---: | --- |
| `libSDL3.a` | 12,745,104 | 12.15 | SDL3; `include/SDL3/SDL_version.h` |
| `libopenal.a` | 54,610,912 | 52.08 | OpenAL Soft; `lib/cmake/OpenAL/OpenALConfig.cmake` |
| `libavcodec.a` | 3,454,064 | 3.29 | FFmpeg; `include/libavcodec/version.h` |
| `libavformat.a` | 1,974,632 | 1.88 | FFmpeg; `include/libavformat/version.h` |
| `libavutil.a` | 2,458,344 | 2.34 | FFmpeg; `include/libavutil/version.h` |
| `libswscale.a` | 1,903,056 | 1.81 | FFmpeg; `include/libswscale/version.h` |
| `libswresample.a` | 348,560 | 0.33 | FFmpeg; `include/libswresample/version.h` |
| `libpng16d.a` | 923,400 | 0.88 | libpng; `include/png.h` |
| `libzd.a` | 681,824 | 0.65 | zlib-ng compatibility build; `include/zlib.h` |

The link also uses `_deps/libdwarf-build/.../libdwarf.a`, four ztd-family static libraries, locally built cpptrace/fmt/glm/ImGui/LuaJIT/spdlog/GLAD archives, and Apple frameworks. The installed Apple toolchain/SDK and system frameworks remain external by design. No third-party `.dylib` appeared in `otool -L` for this built MM7 executable.

## Nested configure-time source fetches observed locally

The cpptrace route uses the exact libdwarf-lite SHA in `thirdparty/cpptrace/cmake/OptionVariables.cmake`; the six ztd repositories are specified as floating `main` in `thirdparty/ztd_text/CMakeLists.txt`. The SHA below is only the current cached checkout, so it is **not a durable pin** until this fork owns it. Sizes are local checked-out source trees. Some ztd-family license files use the same non-commercial/commercial wording as ztd.text; audit the exact terms and notices before vendoring.

| Cached source under `build/_deps/` | Current SHA | Local MiB | Origin / local license evidence |
| --- | --- | ---: | --- |
| `libdwarf-src` | `5dfb2cd2aacf2bf473e5bfea79e41289f88b3a5f` | 4.0 | `jeremy-rifkin/libdwarf-lite`, pinned in cpptrace; `COPYING` says LGPL-2.1 |
| `ztd.cmake-src` | `382e34f32a4192dd8a3a15e326b155b32d1f60c4` | 0.3 | `soasis/cmake`, floating `main`; `LICENSE.md` needs review |
| `ztd.idk-src` | `a8f2f9dfe6afc68e28d7bc40254259743b48bb45` | 3.4 | `soasis/idk`, floating `main`; `LICENSE.md` needs review |
| `ztd.inline_containers-src` | `80c5acbe8256c1802b3b0c006cbc0e0f61fea534` | 0.3 | `soasis/inline_containers`, floating `main`; `LICENSE.md` is public-domain dedication |
| `ztd.encoding_tables-src` | `c0c5de5a45dee2fc3a8a3c6831d7070acae435ad` | 3.2 | `soasis/encoding_tables`, floating `main`; `LICENSE.md` needs review |
| `ztd.cuneicode-src` | `8ed6b88ee41a0edc398db8dbc53d61a6bf6a8f8f` | 5.2 | `soasis/cuneicode`, floating `main`; `LICENSE.md` and `NOTICE` |
| `ztd.platform-src` | `d03b15049adb00ffb67c0f46b888f2a611ecf21f` | 0.3 | `soasis/platform`, floating `main`; `LICENSE.md` |

## Migration sequence and acceptance

1. **Freeze the working MM7 baseline.** Record the title-locked source commit, app binary hash, current arm64 archive hash above, toolchain/SDK, and a private backup of the current profile/save. Leave the protected MM7 worktree and installed Atlas helper alone.
2. **Resolve provenance and license records before copying.** Obtain source revisions and build recipes for all five prebuilt families, especially FFmpeg configuration and any transitive codecs. Review ztd-family license/NOTICE wording and libdwarf LGPL obligations. Decide whether to retain each source or replace its use. Record original URL, exact commit/archive digest, license and any patches for every chosen input.
3. **Own the source inputs in this repo.** Add pinned source snapshots with licenses/notices and a machine-readable provenance manifest; include the 20 Mac app-configure submodule sources and seven nested fetches. Keep optional style/test inputs separately identified. Replace `CMakeModules/Git.cmake`'s unconditional update and all active `FetchContent` fetches with local-only paths; fail clearly on a missing input. A clean worktree with network disabled must not contact any OpenEnroth or other hosting service.
4. **Build the five archive families locally.** Replace the release ZIP route with repo-owned, pinned source recipes for SDL3, OpenAL Soft, FFmpeg, libpng and zlib-ng. Set explicit CMake target paths/options and record the new outputs and license notices. Preserve the existing Mac linkage and decode/audio/render behavior; do not substitute a Homebrew cache or silently fall back to `/usr/X11R6` (the current Debug CMake cache's `PNG_LIBRARY_RELEASE` points there despite its Debug archive selection).
5. **Prove from a clean clone.** With empty build/cache directories and network denied, configure Debug and a distribution configuration with tests/tools off as appropriate, build `MM7.app`, check architecture, `otool -L`, package notices and exact build-input manifest. Stage Atlas with the new helper only after an isolated MM7 launch, existing-save load, new save, quit/relaunch/load and graphical checks pass. Keep MM6/MM8 sibling apps and profiles untouched.

This gate removes build-time reliance on OpenEnroth-hosted artifacts while retaining OpenEnroth Git history, LGPL license and credit. It does not make the Apple SDK or the owner's lawful MM7 game data part of this repository. The observed cached build is useful evidence, not proof that the migration or an offline clone works.
