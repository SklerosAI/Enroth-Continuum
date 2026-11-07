---
type: synthesis
up: "[[Runtime-Isolation]]"
related: ["[[Source-and-Privacy-Boundary]]", "[[Agent-Handoff]]"]
tags: [mm7, build, ownership, attribution]
---

# MM7 Self-Contained Source

The [source-autonomy packet](../project/MM7_SELF_CONTAINED_SOURCE.md) sets a later gate for building and packaging MM7 entirely from SklerosAI's checkout while offline.

The current Atlas helper already comes from this fork's MM7 source, but a normal clean build may download OpenEnroth-hosted prebuilt dependencies. The [local arm64 build-input inventory](../project/MM7_SELF_CONTAINED_BUILD_INVENTORY.md) recorded one 21.7 MB archive with nine linked static libraries, 23 original gitlinks (about 126 MiB of checked-out trees), and seven cached nested source checkouts. Pinned `generator`, `fast_float`, `magic_enum`, `mio`, `small_vector`, `cmrc`, `cli11`, `fmt`, `spdlog`, `sol2`, GoogleTest, `gtest_parallel`, `luajit-cmake`, LuaJIT, Boost.Lambda2, cpptrace, cpplint, GLM and Dear ImGui sources are now tracked here with full licenses and provenance, leaving one external ztd.text gitlink. GoogleTest and `gtest_parallel` are optional test inputs. The owned LuaJIT source and CMake recipe now generate the same version header without nested Git. The first sixteen snapshots passed standalone checks; all nineteen now pass a cached-input arm64 MM7 link. The latest three also passed exact Git blob/mode comparison and focused C++/Python syntax checks. The normal `CMakeModules/Git.cmake` path runs a recursive submodule update, cpptrace fetches a pinned libdwarf-lite commit, and ztd.text fetches six floating `main` branches. The prebuilt ZIP has no complete source/license manifest or expected hash in the CMake download rule; ztd-family license wording and FFmpeg build terms need review before copying. The inventory is local evidence, not an offline-clone test.

An opt-in `EC_LOCAL_ONLY_CONFIGURE=ON` gate now skips the recursive submodule update, forbids the OpenEnroth dependency archive, disconnects nested FetchContent and fails with named missing repository-owned inputs. A fresh arm64 AppleClang configure reached that refusal without a download; focused fake-Git and archive-guard checks passed. The gate deliberately refuses even if directories exist until pinned local source-build recipes are wired. It is a fail-closed migration scaffold, not an offline MM7 build. The normal configure path and installed helper remain unchanged.

Boost.Lambda2's exact pinned files identify the Boost Software License but did not include its full text. The repository adds an unmodified copy from [Boost's official license page](https://www.boost.org/LICENSE_1_0.txt) as separately marked metadata, with the source URL and hash in its provenance file. The integration cached-input MM7 target still builds.

The owned cpptrace snapshot includes its MIT license. Its current Mac backend still fetches pinned LGPL libdwarf-lite; the standalone source build used an alternate backend and does not close that production fetch. The cached-input integration MM7 target builds, while the local-only configure lane still refuses missing inputs.

The five source families behind the Mac prebuilt archive are now ordinary tracked files under [`thirdparty/repo-source/`](../thirdparty/repo-source/README.md), at the dependency tag's exact commit pins. [`SOURCES.json`](../thirdparty/repo-source/SOURCES.json) records source URLs, archive hashes and primary license files; all 11,835 regular archive files matched the extracted bytes. Inherited ignore rules initially omitted 84 files, which were force-added, and Git attributes prevent line-ending normalization. No source-build recipe is connected yet, so this does not change the current MM7 link or the local-only refusal. License/notice packaging and FFmpeg's effective component set remain open.

Scoped production headers and required CMake files from pinned inifile-cpp, tcpp and nlohmann/json are also tracked with licenses and provenance, replacing three more gitlinks. All 72 copied upstream files matched pinned Git blobs. The cached-library MM7 link, 12 focused JSON/preprocessor units and a headless MM7 control passed. Their upstream test-only Catch2 and GPL files were not copied. Only the ztd.text gitlink remains; its pinned license wording and six nested fetches require an owned Mac encoding strategy before the local-only build can be complete.

The [upstream dependency release](https://api.github.com/repos/OpenEnroth/OpenEnroth_Dependencies/releases/tags/deps_r8_master) publishes a size and SHA-256 that match this Mac's cached arm64 ZIP. Its [pinned tag](https://api.github.com/repos/OpenEnroth/OpenEnroth_Dependencies/git/refs/tags/deps_r8_master) and [tree](https://api.github.com/repos/OpenEnroth/OpenEnroth_Dependencies/git/trees/866c037e528bb38ee286874f107fb488dd9fb354?recursive=1) identify exact FFmpeg, libpng, OpenAL Soft, SDL and zlib-ng revisions; the build script gives a recipe starting point. The [inventory](../project/MM7_SELF_CONTAINED_BUILD_INVENTORY.md) records those pins. They do not replace source-license/notice review or a repo-owned build recipe. The upstream recipe repository lacks a top-level license, so create our own recipe from the verified facts.

The pinned source audit now records each family's primary license file and additional bundled-component notices in the inventory. FFmpeg's cached archives embed the tagged recipe's GPL/version3/nonfree disables and report LGPL-2.1-or-later, but the recipe permits automatic external-library detection and the archive lacks `config.h` or a build log. A reproducible replacement must pin effective features and package the required notices; no source-build recipe was committed in this pass.

Atlas and direct Mac helper source now default MM7 data and user paths to private Enroth Continuum roots, with Vault and transfer-path access on the fresh profile. The new private data root holds a byte-verified copy of extracted MM7 data, and a headless control passes through that default. The [runtime path cutover](MM7-Runtime-Path-Cutover.md) requires separate private backups and collision review before moving saves or switching the installed app. Legacy locations remain rollback sources; neither game data nor saves enter Git.

The phased route is to preserve OpenEnroth credit, record exact provenance and license obligations, put approved pinned source/recipes in this repo, remove configure-time network actions, build SDL3/OpenAL Soft/FFmpeg/libpng/zlib-ng from those sources, then prove a clean network-denied arm64 MM7 build and Atlas launch/save/reload from the explicit private paths. Completion requires the fresh-clone build to use no OpenEnroth checkout, dependency release, submodule origin, build-time URL or local dependency cache. Original GOG data and saves stay private and outside Git. The Apple SDK remains an external system input.

The source migration remains incomplete. An optional `upstream` remote remains only for deliberate review of later source changes; OpenEnroth history, license, notices and authorship remain in the fork. Do not replace the known-good installed MM7 helper before the new build passes its play and save checks, and do not describe a cached local build as a clean offline build.

The owner reaffirmed this as a required eventual MM7 finish line. The gate now
also calls for an audit of user-facing app identity, bundle metadata, Atlas
text, CLI defaults and local config/cache/log paths, so the installed helper
uses Enroth Continuum/MM7 identity and private paths throughout. Historical
OpenEnroth links and authorship remain in credits and source provenance;
internal inherited symbol names can be migrated separately when safe.

See also: [Runtime Isolation](Runtime-Isolation.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Agent Handoff](Agent-Handoff.md).
