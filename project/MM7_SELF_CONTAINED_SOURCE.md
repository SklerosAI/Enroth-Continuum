# MM7 source autonomy in SklerosAI

Status: **planned** (2026-11-06). This is the owner's later MM7 repository-ownership gate, not a claim that a clean offline clone builds today.

## Current boundary

The `MM7` target and Atlas helper are compiled from Enroth Continuum's own forked source. Atlas stages an explicitly supplied, title-locked `MM7.app`; it does not download or run an OpenEnroth release binary. OpenEnroth's Git history, LGPL license, notices and contributor credit remain part of the fork.

A clean Mac build is not yet self-contained. `CMakeModules/Dependencies.cmake` downloads a versioned archive from `OpenEnroth/OpenEnroth_Dependencies` when the prebuilt cache is absent. The checkout also has 23 externally hosted Git submodules, and the non-prebuilt route may fetch additional source at configure time. Cached local dependencies hide those requirements on this Mac. The upstream Git remote is optional for reviewed synchronization; it is not a runtime input.

## Migration gate

1. Freeze the working MM7 source revision, installed helper, save/profile backup and build hashes. Keep Atlas pointed at the known-good helper until the replacement passes launch and save/load checks.
2. Record each Mac MM7 build input, exact revision/hash, license, current fetch path, size and whether it is required at runtime. Keep original Might & Magic game data and saves outside Git.
3. Replace OpenEnroth-hosted prebuilt dependency downloads with reproducible source builds from pinned, license-preserving inputs held in this repository. Replace required Mac-build submodule/fetch-at-configure dependencies with repository-owned source snapshots. Do not remove upstream copyright files or flatten authorship without a traceable provenance record.
4. Make the build fail clearly when a required input is absent; it must not silently fetch from OpenEnroth or substitute a cached local library. Produce the arm64 `MM7.app` and stage Atlas from the clean SklerosAI checkout while offline, using the installed Apple toolchain/SDK.
5. Check the app's title lock, notices and library links, then launch MM7 through Atlas, load an existing save, save again, quit and reload. Confirm the MM6/MM8 sibling slots and private profiles remain untouched.

The completion claim means the **Mac MM7 engine and its build inputs** are owned by this repo. The Apple toolchain/SDK and the owner's lawfully obtained GOG game files remain external by design. Attribution to OpenEnroth is permanent provenance, not a build dependency.

See [the implementation plan](plan.md) and [upstream relationship](UPSTREAM.md).
