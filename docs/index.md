# Docs index

Every page in `docs/` with a one-line summary, by category. Read this before searching.

## Start here

- [README](README.md): home, reading order, and where the old note trees went.
- [Schema](SCHEMA.md): how pages are written and kept current. Source and tests outrank these pages.
- [Log](log.md): dated milestones, one line each.
- [Project overview](project-overview.md): what Enroth Continuum is, title boundaries for MM6, MM7 and MM8,
  priorities and ownership.

## Status

- [MM6 campaign status](mm6-campaign-status.md): what an MM6 player can do today, how it is verified, chain and
  GrayFace progress, and the latest lane results.
- [MM7 status](mm7-status.md): MM7 working gate, endings, promotions, quest durability, audit fixes and what remains.
- [MM6 playthrough go/no-go](mm6-playthrough-go-no-go.md): whether MM6 is ready for an end-to-end playthrough, the
  player-reachable crashes that block it and what flips the verdict.
- [Open questions](open-questions.md): open questions and known bugs across MM6 and MM7.
- [Releases](releases.md): release tags, their provenance, how a release is built and checked.

## Engine

- [Engine architecture and runtime map](engine-architecture.md): binaries, startup, game loop, title selection,
  asset loading, rendering, saves and events, and where each lives in `src/`.

## MM6 port

- [MM6 port architecture](mm6-port-architecture.md): where MM6 code lives, title gating, the `gameplay.fix_*` flag
  convention, `ForTest` seams and a tour of `src/Engine/MM6`.
- [MM6 rules from MM6.exe](mm6-executable-rules.md): `mm6::Executable`, the accepted executable, and the rule tables
  decoded from it.
- [MM6 characters, items and spells](mm6-characters-items-spells.md): classes, creation, skills, conditions, items,
  potions, spells, buffs and awards.
- [MM6 events](mm6-events.md): event script format, opcodes that differ from MM7, variables and triggers.
- [MM6 combat and monsters](mm6-combat-and-monsters.md): monster records, attacks, spells, hostility, party combat,
  turn-based mode and party death.
- [MM6 houses and services](mm6-houses-and-services.md): house routing, shops, guilds, temples, training, taverns,
  banks, transport, hirelings, street NPCs and resting.
- [MM6 travel and maps](mm6-travel-and-maps.md): map formats, terrain, sky, borders, transport, parked maps, timers,
  doors, decorations and chests.
- [MM6 media and UI](mm6-media-and-ui.md): main menu, movies, HUD, minimap, portraits, voice and cursors.
- [MM6 save format](mm6-save-format.md): the `.ec6` container, its sections and versions, save and load paths,
  autosaves and atomic writes.
- [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md): GrayFace Patch 2.5.7 fixes cross-referenced against
  our runtime, the fix-them-all decision and the data-fix inventory.

## Tooling

- [Testing](testing.md): unit and game tests, tapes, data-path env vars, hidden captures and the gate runners.
- [Build and dev setup on macOS](build-and-dev-setup.md): toolchain, targets, style checks, the offline MM7 build and
  local game data.
- [Mods and the Atlas launcher](mods-and-atlas.md): the Atlas app, unified Fortune rerolls, MM6 All Artifacts and private MM7 mods.

## Process

- [Agent workflow](agent-workflow.md): Codex and Claude ownership, lanes and worktrees, integration branches, the live
  mailbox, and where results are recorded.
- [Upstream relationship](upstream.md): the OpenEnroth upstream, attribution and how upstream changes come in.
- [Source and privacy](source-and-privacy.md): what never goes in git, `local-game-data/`, and how tools find
  private data.
