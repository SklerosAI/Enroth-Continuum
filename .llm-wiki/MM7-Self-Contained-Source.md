---
type: synthesis
up: "[[Runtime-Isolation]]"
related: ["[[Source-and-Privacy-Boundary]]", "[[Agent-Handoff]]"]
tags: [mm7, build, ownership, attribution]
---

# MM7 Self-Contained Source

The [source-autonomy packet](../project/MM7_SELF_CONTAINED_SOURCE.md) sets a later gate for building and packaging MM7 entirely from SklerosAI's checkout while offline.

The current Atlas helper already comes from this fork's MM7 source, but a normal clean build may download OpenEnroth-hosted prebuilt dependencies. The [local arm64 build-input inventory](../project/MM7_SELF_CONTAINED_BUILD_INVENTORY.md) recorded one 21.7 MB archive with nine linked static libraries, 23 original gitlinks (about 126 MiB of checked-out trees), and seven cached nested source checkouts. Pinned `generator`, `fast_float`, `magic_enum`, `mio`, `small_vector`, `cmrc`, `cli11`, `fmt`, `spdlog`, `sol2`, GoogleTest, `gtest_parallel` and `luajit-cmake` sources are now tracked here with full licenses and provenance, leaving 10 external gitlinks. GoogleTest and `gtest_parallel` are optional test inputs. The owned LuaJIT CMake recipe still runs `git show` in its separate LuaJIT source gitlink; that version step must be replaced when LuaJIT source is owned here. Standalone network-disconnected checks and the cached-input Mac MM7 target passed. The normal `CMakeModules/Git.cmake` path runs a recursive submodule update, cpptrace fetches a pinned libdwarf-lite commit, and ztd.text fetches six floating `main` branches. The prebuilt ZIP has no complete source/license manifest or expected hash in the CMake download rule; ztd-family license wording and FFmpeg build terms need review before copying. The inventory is local evidence, not an offline-clone test.

An opt-in `EC_LOCAL_ONLY_CONFIGURE=ON` gate now skips the recursive submodule update, forbids the OpenEnroth dependency archive, disconnects nested FetchContent and fails with named missing repository-owned inputs. A fresh arm64 AppleClang configure reached that refusal without a download; focused fake-Git and archive-guard checks passed. The gate deliberately refuses even if directories exist until pinned local source-build recipes are wired. It is a fail-closed migration scaffold, not an offline MM7 build. The normal configure path and installed helper remain unchanged.

Atlas currently defaults MM7 data, saves, saved-folder access and character transfer to legacy `OpenEnroth` local locations. The [source-autonomy gate](../project/MM7_SELF_CONTAINED_SOURCE.md) also requires an explicit private Enroth Continuum data root and profile for those runtime paths, after a verified local copy of the owner's existing files. The legacy locations remain a rollback source until launch and save/reload checks pass; neither game data nor saves enter Git.

The phased route is to preserve OpenEnroth credit, record exact provenance and license obligations, put approved pinned source/recipes in this repo, remove configure-time network actions, build SDL3/OpenAL Soft/FFmpeg/libpng/zlib-ng from those sources, then prove a clean network-denied arm64 MM7 build and Atlas launch/save/reload from the explicit private paths. Completion requires the fresh-clone build to use no OpenEnroth checkout, dependency release, submodule origin, build-time URL or local dependency cache. Original GOG data and saves stay private and outside Git. The Apple SDK remains an external system input.

The source migration remains incomplete. An optional `upstream` remote remains only for deliberate review of later source changes; OpenEnroth history, license, notices and authorship remain in the fork. Do not replace the known-good installed MM7 helper before the new build passes its play and save checks, and do not describe a cached local build as a clean offline build.

The owner reaffirmed this as a required eventual MM7 finish line. The gate now
also calls for an audit of user-facing app identity, bundle metadata, Atlas
text, CLI defaults and local config/cache/log paths, so the installed helper
uses Enroth Continuum/MM7 identity and private paths throughout. Historical
OpenEnroth links and authorship remain in credits and source provenance;
internal inherited symbol names can be migrated separately when safe.

See also: [Runtime Isolation](Runtime-Isolation.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Agent Handoff](Agent-Handoff.md).
