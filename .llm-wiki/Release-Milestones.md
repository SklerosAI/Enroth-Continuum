---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [release, mm7, atlas, provenance]
related: ["[[Campaign-Status]]", "[[Runtime-Isolation]]", "[[MM7-Playtest-Recording]]"]
---

# Release Milestones

## Local MM6/MM7 playtest app — 2026-11-22

The owner authorized a new local app build and installation. Atlas now bundles
separately built Release arm64 MM6 and MM7 helpers from their continuing title
branches. This is a local playtest update: no release tag was created or moved,
and MM6 remains a playtest candidate.

| Helper | Clean source commit | Source tree | Installed executable SHA256 |
| --- | --- | --- | --- |
| MM7 | `01451425068e5699deae318680f1673d24ad9005` | `df6aa1d2d4336ee78b9dcc5c0dbef2476457ef9e` | `bd89d30901b2b3f93d8d59ee357aace2a67e7c435d8bcb2a2e955b7b5b4eee54` |
| MM6 | `626f5f431015096adaa4b89f9d54aae92d3d7c1a` | `0fcb761c9f4fa39bf7619270ed54c2e616a3ff9f` | `ab4deb72a5f385cbb877c249b9a0e37ea375ed75bb7739da9d3f41bd51c429e7` |

The MM6 mod implementation is `27019e9fa`; the later source commit includes
its verified documentation. Atlas carries helper build-info records and
existing media notices. Both helpers link Apple system OpenGL and system
runtime libraries only. The MM6 Release build additionally pins its existing
local static PNG archive; neither helper requires XQuartz or a package-manager
runtime library. These local builds reuse existing static dependency caches;
they are not new clean-offline-source-build evidence.

The merged [Fortune’s Turn](MM7-Monster-Loot-Reroll-Mod.md) choice enables
chest reroll and the other reward rerolls for each title. Chest Reroll stays
available with the in-game X off; X controls non-chest rewards. MM6 has a
separate All Artifacts choice. The installed Atlas visibly showed both MM6
choices On, campaign data ready and the MM6 Play action available.

### Verification and remaining playtest

- The full MM6 headless suite passed 412 cases with no skips, the fresh-process
  save/load gate passed 44 checks, and the C0–C9 campaign chain passed with
  Fortune’s Turn and All Artifacts enabled. Native controls and a hidden HUD
  capture also passed. These checks do not close visible campaign acceptance.
- Fresh MM7 focused checks passed eight Dragoon cases and 39 mod-coexistence
  cases, with one expected off-only skip. Recorder checks passed 29/29;
  launcher Swift contracts, Atlas page tests, full Swift typecheck and changed
  C++ lint passed. The unrelated 749-case default sweep was intentionally
  stopped; no new complete MM7 suite result is claimed.
- The installed app matched the staged candidate’s 45 files and symlink
  entries, and deep strict code-sign verification passed. The visible check
  covered Atlas Campaigns and Mods, not interactive game play or an installed
  helper’s normal Save/quit/relaunch/Load transaction.
- The prior installed app was deleted directly without retaining a backup.
  Superseded helper app artifacts and the final outer staging bundle were
  removed after verification. Current Release helper outputs remain as build
  products. Profiles, saves and original game data were not removed.

Continue with owner MM6 playtesting: opening, travel, dungeon interaction,
chest button with X off, non-chest rerolls with X on, and normal save/reload.
[Runtime Isolation](Runtime-Isolation.md) defines the helper/profile boundary;
the [Agent Handoff](Agent-Handoff.md) records the current source checkpoint.

## Local MM7 UI update after ECv0.1.1 — 2026-11-20

The installed Atlas still reports 0.1.1, but its MM7 helper now contains the
themed reroll controls from commit `bfb4dba6038c57c7b9c5351602071d6268ffcbaf`.
The `ECv0.1.1` tag was not moved. The chest button sits inside the lower wooden
frame, and the loot popup and HUD dice use MM7-style brass, dark wood and gold
lettering. The popup shows the game world behind its stone frame. The MM6
helper was carried forward byte-for-byte.

The full MM7 headless suite passed 737/737, the focused reroll tests passed
4/4, `check_style` passed, and hidden OpenGL captures of the chest, popup and
HUD were inspected. A revert-and-fail check confirmed the popup-world capture
fails when the drawing change is removed and passes when restored. The new
Release arm64 MM7 helper was staged with a build-info record naming the
post-tag commit; the staged and installed Atlas bundles passed deep strict
signature verification. The old app was removed without retaining a backup.
Game data, profiles and saves were left outside the app. Visible installed-app
interaction remains for owner playtesting.

A further local update at commit `ed28816ff014387ce0f89fcde76b2f8097828118`
expanded the click-loot popup to show the rolled item's full name, attack or
armor stats, magic bonus, value and description on screen. The preview reads
these values without marking the item identified. Its focused MM7 tests passed
4/4, `check_style` passed, and a hidden OpenGL popup capture was inspected.
The signed Release arm64 Atlas was reinstalled with this commit in its MM7
build-info record and the same MM6 helper. The previous app was removed, while
profiles and saves remained outside the bundle. The full 737-test suite above
was run for the earlier themed update; it was not repeated for this popup-only
follow-up. The `ECv0.1.1` tag remains fixed.

## ECv0.1.1 — MM7 patch — 2026-11-20

`ECv0.1.1` is an annotated public tag on commit
`e7ed060979d2942f92f0dd10f312914a3efd0947`, tree
`54e392993be87d53d30211d57af135c869af8879`. The tag and `master`
source history were pushed to origin. It adds separate opt-in MM7 chest and
click-loot rerolls, including a shared in-game checkbox for random monster
corpses and clicked random scripted items. Fixed rewards and gold retain their
original path. The MM7 helper and Atlas now report version 0.1.1.

The source also builds a title-locked MM6 helper for owner playtests. MM6 is
still not a released or campaign-complete title. Claude's active `claude/mm6`
worktree remains separate, and its later work was not merged into this patch.

### Release evidence

- Full default MM7 headless suite: 737/737 passed. Opt-in mod combination:
  5/5 passed. Unit tests: 1125 passed, 45 expected private-evidence skips.
- MM6 item-event and decoration controls: 3/3 passed. `check_style`, Atlas
  page and Swift build-contract tests passed.
- All 28 MM7 fresh-process checkpoint pairs passed from the clean candidate.
  Both light and dark C0–C7 campaign chains passed from new temporary profiles,
  which were removed afterward.
- Two hidden MM7 OpenGL captures passed. The mod-enabled opening HUD capture
  was inspected and showed the checkbox and dice mark beside the first portrait.
- The exact-tag Release arm64 build used the pinned local media recipe. The
  combined Atlas bundle contains MM7 and MM6 helpers with tag, commit and tree
  build-info records; both helper binaries identify `ECv0.1.1`. The staged
  and installed bundles match recursively and pass deep strict signature
  verification. The prior installed app and old app-only rollback copies were
  removed without making a new backup. Private game data and saves stayed put.

Graphical popup interaction, an installed-app game launch and an installed-app
Save/Load remain for owner playtesting. The full MM7 map, spell, item, monster
and archive audit remains planned separately.

## ECv0.1.0 — MM7 — 2026-11-16

`ECv0.1.0` is the first published Enroth Continuum milestone and the MM7
release. It is an annotated public Git tag whose release commit is
`50d7d4197eece1805866a2cc33977f44b9a2dd87`, tree
`dd124fb2abdf302eaeb2beb6dfeae0b8bf63a4d1`. The tag is not moved by later
documentation updates on `master`.

This milestone contains the integrated MM7 campaign, quest, promotion, map,
movement/combat, Barrow, crash-diagnostic and optional-mod work summarized in
[[Campaign-Status]]. The same source snapshot includes ongoing MM6 work and a
title-isolated MM6 helper, but MM6 remains a playtest candidate rather than a
released or campaign-playable title. MM8 remains reserved, not implemented.

### Pre-tag evidence

- A fresh combined build completed.
- All 158 style targets passed.
- The focused MM6 gameplay selection passed 60/60.
- The focused unit selection passed 26 cases; 14 private-fixture cases skipped
  as expected.
- Six active MM7 movement/combat controls passed.
- Seven active MM7 Vendor Refresh and All Artifacts + Storage controls passed.
- The preceding consolidated MM7 checkpoint passed 1,124 unit tests, 721
  default GameTests, eight opt-in mod tests, all admitted durability pairs,
  both fresh C0-C7 campaign chains and both hidden visual captures.

### Exact-tag build and local installation

The clean tagged source was built as Release arm64 with the pinned dependency
recipe. The combined Atlas candidate contains the MM7 helper and an MM6
playtest helper. Generated build-info records name the tag, full commit and
tree. Deep strict code-sign verification passed, the installed bundle matched
the staged candidate recursively, and The Atlas rendered visibly with both
campaign cards and their data/helper states. Neither game was launched during
that final installation check, and no profile, save, mod or recording data was
changed.

The superseded application was sent to macOS Trash at the owner's direction;
there is no project rollback archive. The installed application is the sole
active `/Applications` copy.

### Known release limits

- Publication is the annotated Git tag and pushed source history; no separate
  GitHub Release page or binary asset was created.
- The outer Atlas `Info.plist` reports short version `0.1` and build `1`, while
  the release tag and helper provenance report `0.1.0`. Preserve the published
  tag and correct the packaging metadata in a subsequent release.
- The final installation check verified Atlas rendering and exact bytes, not a
  post-install game launch or save/reload transaction.
- The new ad-hoc signature may require macOS Screen Recording permission to be
  granted again before rolling video capture works.

See [[Runtime-Isolation]] for title boundaries, [[MM7-Playtest-Recording]] for
the recording/diagnostic contract, and the [Claude release handoff](../agent_comms/messages/2026-11-16T19-54-52Z--from-codex--to-claude--ecv0.1.0-and-mm6-integration.md)
for the integrated and preserved MM6 lanes.
