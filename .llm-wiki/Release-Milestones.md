---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [release, mm7, atlas, provenance]
related: ["[[Campaign-Status]]", "[[Runtime-Isolation]]", "[[MM7-Playtest-Recording]]"]
---

# Release Milestones

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
