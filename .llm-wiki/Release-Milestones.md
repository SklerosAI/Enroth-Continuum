---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [release, mm7, atlas, provenance]
related: ["[[Campaign-Status]]", "[[Runtime-Isolation]]", "[[MM7-Playtest-Recording]]"]
---

# Release Milestones

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
