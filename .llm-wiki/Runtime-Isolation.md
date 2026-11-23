---
type: concept
up: "[[index_Enroth-Continuum]]"
tags: [architecture, launcher, saves]
---

# Runtime Isolation

The installed Atlas places title-locked MM6 and MM7 engine helpers side by side,
each with its own local profile. MM8 remains reserved and disabled.

## Installed app

The [local playtest app](Release-Milestones.md#local-mm6mm7-playtest-app--2026-11-22)
contains Release arm64 MM7 from `codex/mm7` source `014514250` and MM6 from
`claude/mm6` source `626f5f431`. Both use Apple system OpenGL and system
runtime libraries, pass title-lock checks and deep strict signature
verification. Installed on 2026-11-22. A verified replacement deletes the old
app, so no app backup is kept.

`MM6.app` and `MM7.app` are separate helper slots under Atlas. MM6 uses
`Profiles/MM6-Atlas`; MM7 uses `Profiles/MM7-Atlas`. Atlas clears the other
title’s mod flags and keeps upstream OpenEnroth free of fork mod flags. MM6
has its own All Artifacts and merged Fortune’s Turn choices; MM7 applies its
own choices through Private mods. The HUD X controls non-chest reward rerolls,
while the chest’s Reroll button remains available with X off. MM6 does not
use the MM7 recording collector. Visible Atlas rendering and launch readiness
passed. MM6 on this build is not ready for an end-to-end playthrough: reading a
spellbook ends the game, and the blockers are tracked in
`docs/mm6-playthrough-go-no-go.md` on `claude/mm6`. The MM7 verdict is in
[MM7 Playthrough Go/No-Go](MM7-Playthrough-Go-No-Go.md).

The [Atlas assembler](../launcher/assemble.py) validates bundle identity,
arm64 code and foreign-title rejection, carries existing siblings and media
notices from an explicit Atlas input, and stages outside Applications. Its
build-info records identify each helper’s own source commit and tree.
The title branches are `codex/mm7` and `claude/mm6`. `mm8-isolated-runtime`
identifies a reserved baseline, not an implemented runtime.

Profiles, game installations, saves, backups, and mods remain local. The Atlas is intended to show true per-title readiness and to keep the original campaign experience available alongside optional private mods and eventual transfer features.

The later [MM7 self-contained source gate](MM7-Self-Contained-Source.md) makes a clean Mac build independent of OpenEnroth-hosted dependency releases and external Git submodules, with permanent upstream attribution. The installed helper still uses the cached prebuilt libraries.

Source: [project/LOCAL_STATUS.md](../project/LOCAL_STATUS.md), [project/ROADMAP.md](../project/ROADMAP.md), [launcher/README.md](../launcher/README.md), reviewed for the installed checkpoint on 2026-11-22.

See also: [Campaign Status](Campaign-Status.md), [MM7 Self-Contained Source](MM7-Self-Contained-Source.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Project Overview](Project-Overview.md), [Agent Handoff](Agent-Handoff.md).
