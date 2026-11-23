# Releases

Enroth Continuum's release tags, their provenance, how a release is built and checked, and the packaging follow-ups.

## Release line

| Tag | Title | State |
| --- | --- | --- |
| `ECv0.1.0` | MM7 | Published 2026-11-16 |
| `ECv0.1.1` | MM7 patch | Published 2026-11-20 |
| `ECv0.2.0` | MM6 | Planned, date undecided. MM6 work collects on `claude/mm6` until then. |
| `ECv0.3.0` | MM8 | Planned after MM6 |

A release is an annotated Git tag on pushed source history. No GitHub Release page or binary asset is published, and
no release ever contains game data. A published tag is never moved or rewritten. Later documentation commits on
`master` do not change it.

## Tags in the repository

| Tag | Kind | Points at | Meaning |
| --- | --- | --- | --- |
| `ECv0.1.0` | Annotated, tag object `9757e16d394f`, pushed to `origin` | Commit `50d7d4197eece1805866a2cc33977f44b9a2dd87`, tree `dd124fb2abdf302eaeb2beb6dfeae0b8bf63a4d1` | The MM7 release |
| `ECv0.1.1` | Annotated, tag object `d5907f2d30e3`, pushed to `origin` | Commit `e7ed060979d2942f92f0dd10f312914a3efd0947`, tree `54e392993be87d53d30211d57af135c869af8879` | The MM7 patch |
| `nightly` | Lightweight, local only | Upstream commit `8010ac364` (2026-11-09) | A stale local copy of upstream OpenEnroth's moving `nightly` tag. Not a fork release. |

Check a tag with `git tag -l` and `git show <tag> --no-patch`.

## ECv0.1.0

The tag message calls it the first playable MM7 milestone: The Atlas, durable MM7 saves, campaign and quest coverage,
movement and combat harnesses, playtest diagnostics and the optional MM7 private mods. The same source carries MM6 work
and a title-locked MM6 helper, but MM6 is a playtest candidate in this release, not a playable campaign. MM8 is not
implemented.

Evidence gathered before tagging:

- A fresh combined build and every style target passed.
- A focused MM6 gameplay selection and a focused unit selection passed, with private-fixture cases skipped as
  expected.
- The active MM7 movement and combat controls and the MM7 vendor and All Artifacts mod controls passed.
- The consolidated MM7 checkpoint before it passed the unit tests, the default game tests, the opt-in mod tests, every
  durable writer and reader pair, both fresh C0 to C7 campaign chains and both hidden capture tests.

## ECv0.1.1

An MM7 patch on `master`. It adds the two opt-in MM7 reroll mods, chest reroll and the click-loot popup that later grew
into Fortune's Turn (see [Mods and Atlas](mods-and-atlas.md)). Fixed rewards and gold keep their original path, and
MM7 keeps the `ECv0.1.0` save and profile paths. The Atlas `Info.plist` and the MM7 helper report `0.1.1`. The source
still builds the title-locked MM6 helper for owner playtests, without the later `claude/mm6` work, and MM6 is not
released.

Evidence gathered before tagging: the full default MM7 game tests, the opt-in mod combination, the unit tests,
`check_style`, the Atlas page and Swift checks, the MM6 item-event and decoration controls, every fresh-process MM7
checkpoint pair, both fresh C0 to C7 chains and two hidden MM7 captures. The exact-tag Release arm64 build carries tag,
commit and tree build-info records for both helpers. The staged and installed bundles matched and passed deep strict
signature verification. Graphical popup interaction, a game launch from the installed app and an installed-app save
and load were left for owner playtesting.

Later local installs carried post-tag MM7 commits into the installed Atlas: the themed reroll controls, the fuller
loot preview and Fortune's Turn. It still reports `0.1.1` and the tag did not move, so the installed MM7 helper is
ahead of the tag. The installed Atlas also carries a Release MM6 helper from `claude/mm6` at `626f5f431` for owner
playtests, which reports `ECv0.1.1-255-g626f5f431`.

## How a release is built and checked

1. **Gate on source.** Run the full build, `check_style`, the unit tests and the title's game tests, plus the other
   title's controls for any shared-code change. The release title's campaign chains and durable save pairs must pass.
2. **Tag.** Create the annotated tag on the release commit and push it.
3. **Build the exact tag.** Build Release arm64 from a clean export of the tagged source with the pinned dependency
   recipe, built from tracked dependency sources. `tools/build_mm7_macos.py` wraps
   `tools/build_mm7_macos_dependencies.py`, configures with the local-only options, checks link inputs and system
   libraries and stages `Atlas/Enroth Continuum.app` with the media source notices. It refuses an existing output root
   and never installs. See [Build and dev setup](build-and-dev-setup.md).
4. **Assemble.** `launcher/assemble.py` (through `launcher/build.sh`) puts each title's helper into the Atlas bundle,
   copies its `BUILD-INFO.txt` beside it as `<title>-BUILD-INFO.txt` and ad-hoc signs it. For a release the build info
   names the tag, the full commit and the tree.
5. **Verify.** Deep strict code-sign verification of the bundle, then a recursive comparison of the staged and the
   installed bundle. Open The Atlas visibly and check every title card shows its data and helper state.
6. **Install.** Replace `/Applications/Enroth Continuum.app` after verifying the replacement. Delete superseded installed
   bundles and staged app artifacts completely. Keep no rollback app copy. Git is the source record. Profiles and
   saves require a separately scoped repair or cleanup.

For `ECv0.1.0` the install check verified Atlas rendering and exact bytes. Neither game was launched in that final
check.

## Packaging follow-ups

- **Post-install launch.** Neither `ECv0.1.0` nor `ECv0.1.1` launched a game from the installed app. The next
  release's install check should launch each playable title and do one save and reload.
- **Screen Recording permission.** The Atlas is ad-hoc signed, so macOS may ask again for Screen Recording after each
  install before the playtest recorder's video works.
- **MM6 helper version.** The `MM6` target sets no version of its own. It inherits the directory's
  `MACOSX_BUNDLE_*_VERSION_STRING`, which is `PROJECT_VERSION`, the output of `git describe`. So the helper reports a
  string like `ECv0.1.1-255-g626f5f431`. `ECv0.2.0` needs explicit version strings like the `MM7` target's.
- **MM6 Release build path.** `tools/build_mm7_macos.py` builds only the MM7 helper. `ECv0.2.0` needs a documented
  Release build of the MM6 helper from a clean export of the tag.

## Source

- [build_mm7_macos.py](../tools/build_mm7_macos.py), [build_mm7_macos_dependencies.py](../tools/build_mm7_macos_dependencies.py)
- [assemble.py](../launcher/assemble.py), [build.sh](../launcher/build.sh), [Info.plist](../launcher/Info.plist)
- [src/Bin/OpenEnroth/CMakeLists.txt](../src/Bin/OpenEnroth/CMakeLists.txt), the helper bundle metadata
- [CREDITS.md](../CREDITS.md)

See also: [Project overview](project-overview.md), [MM7 status](mm7-status.md), [MM6 campaign status](mm6-campaign-status.md), [Build and dev setup](build-and-dev-setup.md)
