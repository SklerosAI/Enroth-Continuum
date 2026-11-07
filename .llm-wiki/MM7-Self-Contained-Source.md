---
type: synthesis
up: "[[Runtime-Isolation]]"
related: ["[[Source-and-Privacy-Boundary]]", "[[Agent-Handoff]]"]
tags: [mm7, build, ownership, attribution]
---

# MM7 Self-Contained Source

The [source-autonomy packet](../project/MM7_SELF_CONTAINED_SOURCE.md) sets a later gate for building and packaging MM7 entirely from SklerosAI's checkout while offline.

The current Atlas helper already comes from this fork's MM7 source, but a clean build may download OpenEnroth-hosted prebuilt dependencies; 23 third-party submodules and nested configure-time fetches are also external. The [local arm64 build-input inventory](../project/MM7_SELF_CONTAINED_BUILD_INVENTORY.md) records the current Debug cache: one 21.7 MB archive with nine linked static libraries, 23 pinned gitlinks (about 126 MiB of local checked-out trees), and seven cached nested source checkouts. `CMakeModules/Git.cmake` runs a recursive submodule update during configure, cpptrace fetches a pinned libdwarf-lite commit, and ztd.text fetches six floating `main` branches. The prebuilt ZIP has no complete source/license manifest or expected hash in the CMake download rule; ztd-family license wording and FFmpeg build terms need review before copying. The inventory is local evidence, not an offline-clone test.

Atlas currently defaults MM7 data, saves, saved-folder access and character transfer to legacy `OpenEnroth` local locations. The [source-autonomy gate](../project/MM7_SELF_CONTAINED_SOURCE.md) also requires an explicit private Enroth Continuum data root and profile for those runtime paths, after a verified local copy of the owner's existing files. The legacy locations remain a rollback source until launch and save/reload checks pass; neither game data nor saves enter Git.

The phased route is to preserve OpenEnroth credit, record exact provenance and license obligations, put approved pinned source/recipes in this repo, remove configure-time network actions, build SDL3/OpenAL Soft/FFmpeg/libpng/zlib-ng from those sources, then prove a clean network-denied arm64 MM7 build and Atlas launch/save/reload from the explicit private paths. Completion requires the fresh-clone build to use no OpenEnroth checkout, dependency release, submodule origin, build-time URL or local dependency cache. Original GOG data and saves stay private and outside Git. The Apple SDK remains an external system input.

This is planned work. An optional `upstream` remote remains only for deliberate review of later source changes; OpenEnroth history, license, notices and authorship remain in the fork. Do not replace the known-good installed MM7 helper before the new build passes its play and save checks, and do not describe a cached local build as a clean offline build.

See also: [Runtime Isolation](Runtime-Isolation.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Agent Handoff](Agent-Handoff.md).
