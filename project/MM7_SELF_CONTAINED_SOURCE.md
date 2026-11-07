# MM7 source autonomy in SklerosAI

Status: **planned; local arm64 inputs inventoried** (2026-11-06). The [build-input packet](MM7_SELF_CONTAINED_BUILD_INVENTORY.md) records the current cached artifacts, 23 gitlinks, seven nested fetched sources, provenance gaps and a phased migration. The owner wants the Mac MM7 engine and every required non-system build input available from the SklerosAI repository itself. This is a later MM7 repository-ownership gate, not a claim that a clean offline clone builds today.

## Current boundary

The `MM7` target and Atlas helper are compiled from Enroth Continuum's own forked source. Atlas stages an explicitly supplied, title-locked `MM7.app`; it does not download or run an OpenEnroth release binary. OpenEnroth's Git history, LGPL license, notices and contributor credit remain part of the fork.

A clean Mac build is not yet self-contained. `CMakeModules/Dependencies.cmake` downloads a versioned archive from `OpenEnroth/OpenEnroth_Dependencies` when the prebuilt cache is absent. Configure also runs an unconditional recursive update for 23 externally hosted Git submodules, and cpptrace/ztd.text fetch seven more source repositories in the observed Mac build. Cached local dependencies hide those requirements on this Mac. The current `launcher/AtlasApp.swift` also defaults MM7 data and saves to legacy `OpenEnroth` locations; its saved-folder and character-transfer actions use that same legacy profile. The upstream Git remote is optional for reviewed synchronization; it is not a runtime input.

## Migration gate

1. Freeze the working MM7 source revision, installed helper, save/profile backup and build hashes. Keep Atlas pointed at the known-good helper until the replacement passes launch and save/load checks.
2. Record each Mac MM7 build input, exact revision/hash, license, current fetch path, size and whether it is required at runtime. Keep original Might & Magic game data and saves outside Git.
3. Replace OpenEnroth-hosted prebuilt dependency downloads with reproducible source builds from pinned, license-preserving inputs held in this repository. Replace required Mac-build submodule/fetch-at-configure dependencies with repository-owned source snapshots. Do not remove upstream copyright files or flatten authorship without a traceable provenance record.
4. Make the build fail clearly when a required input is absent; it must not silently fetch from OpenEnroth or substitute a cached local library. Produce the arm64 `MM7.app` and stage Atlas from the clean SklerosAI checkout while offline, using the installed Apple toolchain/SDK.
5. Check the app's title lock, notices and library links, then launch MM7 through Atlas, load an existing save, save again, quit and reload. Confirm the MM6/MM8 sibling slots and private profiles remain untouched.
6. Give Atlas an explicit MM7 data root and profile under the private `Enroth Continuum` local directories, including its save browser and transfer adapter. Copy and hash-check the owner's existing data and saves locally before changing defaults; retain the legacy locations as a rollback source until the new launch/save/reload path passes. Do not put proprietary files or saves in Git.

The completion claim means a fresh clone can configure, compile and package the **Mac MM7 engine and its required non-system build inputs** with networking disabled, without an OpenEnroth checkout, release archive, submodule origin or build-time URL. Atlas and MM7 use Enroth Continuum's explicit private data/profile paths for launch, saves and transfer. A retained `upstream` Git remote may still be used deliberately to review later changes; it is not consulted by the build or runtime. The Apple toolchain/SDK and the owner's lawfully obtained GOG game files remain external by design. OpenEnroth history, license, notices and contributor credit remain permanent provenance, not build dependencies.

See [the implementation plan](plan.md) and [upstream relationship](UPSTREAM.md).
