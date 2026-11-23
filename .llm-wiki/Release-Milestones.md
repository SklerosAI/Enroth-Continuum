---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [release, mm7, atlas, provenance]
related: ["[[Campaign-Status]]", "[[Runtime-Isolation]]", "[[MM7-Playtest-Recording]]"]
---

# Release Milestones

The published tags and the app installed on the owner's Mac. Tags are annotated, pushed to `origin` and never moved.
Each install replaces the previous app and keeps no backup, under the owner's app replacement policy in
[Agent Handoff](Agent-Handoff.md#app-replacement-policy--2026-11-21).

## Local MM6/MM7 playtest app — 2026-11-22

The installed `/Applications/Enroth Continuum.app` reports version 0.1.1 and bundles two Release arm64 helpers built
from the title branches. No tag was created for this install, and MM6 is a playtest helper, not a released title.

| Helper | Source commit | Source tree | Executable SHA256 |
| --- | --- | --- | --- |
| MM7 | `01451425068e5699deae318680f1673d24ad9005` on `codex/mm7` | `df6aa1d2d4336ee78b9dcc5c0dbef2476457ef9e` | `bd89d30901b2b3f93d8d59ee357aace2a67e7c435d8bcb2a2e955b7b5b4eee54` |
| MM6 | `626f5f431015096adaa4b89f9d54aae92d3d7c1a` on `claude/mm6` | `0fcb761c9f4fa39bf7619270ed54c2e616a3ff9f` | `ab4deb72a5f385cbb877c249b9a0e37ea375ed75bb7739da9d3f41bd51c429e7` |

Each helper's build-info record sits in `Contents/Resources/Helpers/` of the app. Both link Apple system OpenGL and
system runtime libraries only, and neither needs XQuartz. They reuse the local static dependency caches, so they are
not clean offline source builds. Profiles, saves and game data live outside the app, under
`~/Library/Application Support/Enroth Continuum/`.

[Fortune's Turn](MM7-Monster-Loot-Reroll-Mod.md) turns on chest reroll and the other reward rerolls for each title.
The chest's Reroll button works with the in-game X off, and the X controls non-chest rewards. MM6 has a separate All
Artifacts choice.

Only documentation changed on `codex/mm7` after the installed MM7 commit. The playthrough verdict is in
[MM7 Playthrough Go/No-Go](MM7-Playthrough-Go-No-Go.md). Visible installed-game play and an installed Save, quit,
relaunch and Load await the owner.

## ECv0.1.1 — MM7 patch — 2026-11-20

Commit `e7ed060979d2942f92f0dd10f312914a3efd0947`, tree `54e392993be87d53d30211d57af135c869af8879`. It adds opt-in MM7
chest and click-loot rerolls, with a shared in-game checkbox for random monster corpses and clicked random scripted
items. Fixed rewards and gold keep their original path. The tag also builds a title-locked MM6 helper for owner
playtests. Before tagging, the full default MM7 headless suite, the opt-in mod combination, the unit tests, all 28
fresh-process checkpoint pairs and both C0-C7 chains passed on the candidate.

## ECv0.1.0 — MM7 — 2026-11-16

Commit `50d7d4197eece1805866a2cc33977f44b9a2dd87`, tree `dd124fb2abdf302eaeb2beb6dfeae0b8bf63a4d1`. The first published
Enroth Continuum milestone and the MM7 release. It contains the MM7 campaign, quest, promotion, map, movement, combat,
Barrow, crash-diagnostic and optional-mod work summarized in [Campaign Status](Campaign-Status.md). The same snapshot
carries a title-isolated MM6 helper that was a playtest candidate, not a release. MM8 is reserved.

See also: [Runtime Isolation](Runtime-Isolation.md), [MM7 Playtest Recording](MM7-Playtest-Recording.md), [Campaign Status](Campaign-Status.md), [Agent Handoff](Agent-Handoff.md)
