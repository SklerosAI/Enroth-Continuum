# Project overview

What Enroth Continuum is, what it aims for, where each Might and Magic title stands, who owns what and how the
repository is laid out.

## What it is

Enroth Continuum is a source fork of [OpenEnroth](https://github.com/OpenEnroth/OpenEnroth), the open-source
reimplementation of the Might and Magic VI to VIII engine. The fork keeps upstream's Git history, LGPL-3.0-or-later
license and credit (see [CREDITS.md](../CREDITS.md) and [Upstream](upstream.md)). It targets a native Apple Silicon
Mac and the owner's own GOG copies of the games.

The fork aims to run MM7, then MM6, and eventually MM8, each as a complete campaign. Upstream plays MM7 only. MM6 in
this fork is new work on top of the shared engine, with MM6 logic in MM6-owned files and thin, title-gated hooks into
shared code. Original game data never enters the repository (see [Source and privacy](source-and-privacy.md)).

## Goals and end state

- **One installation, three campaigns.** `/Applications/Enroth Continuum.app` opens **The Atlas**, a native launcher
  with a bundled HTML interface. It shows one card per title and launches a separate, title-locked engine helper for
  each playable one.
- **Separate runtimes and profiles.** Each title has its own helper app, data folder and profile under
  `~/Library/Application Support/Enroth Continuum/` (`Game Data/<title>`, `Profiles/MM7-Atlas`, `Profiles/MM6-Atlas`).
  No title reads or writes another title's data, saves, config or mods.
- **Vanilla first.** The original campaigns stay playable without fork features. Mods are opt-in per launch from the
  Atlas Atelier. Bug fixes that change MM6 game logic sit behind `gameplay.fix_*` flags that default on, so a vanilla
  run is one switch away.
- **Durable saves.** Save, quit, relaunch and load must work in the real app, not only in tests.
- **One source tree.** The end state is one worktree with MM6 and MM7 as separate engine code sections in one build.
- **Later, optional.** Cross-game character transfer (a capped, opt-in export and import) comes only after both source
  and destination games work. It is design intent, not code.

## Title boundaries

| Title | Today | Runtime |
| --- | --- | --- |
| MM7 | Playable, released as `ECv0.1.0`. Working gate closed 2026-11-11, full audit in progress. | `MM7` app, `--game mm7`, `EC_MM7_ONLY` |
| MM6 | Playtest candidate. Main-quest chain runs C0 to C6. Planned release `ECv0.2.0`. | `MM6` app, `--game mm6`, `EC_MM6_ONLY` |
| MM8 | Gated. The Atlas card only detects data. No `GameTitle::MM8`, no helper. Planned release `ECv0.3.0`. | None |

MM7's working gate closed with the Light campaign chain to its ending. The Dark chain followed in the audit. MM6 New
Game, New Sorpigal, its dungeons, services, travel, combat and save and load work in tests and owner playtests. MM8
work starts after MM6 and MM7.

The Atlas can also carry an upstream OpenEnroth helper as a second MM7 button for comparison. It uses its own
`Profiles/MM7-OpenEnroth` and never the fork's profile.

Rules for claims about a title:

- A title screen, a party creation screen or a partial map load is not a playable campaign.
- Headless tests alone never close a working gate. A gate also needs a visible run of the current build on the Mac and
  durable save and reload in the real app.
- A "working" gate (play through an ending) and a "full audit" (breadth, optional content, interface and media) are
  separate milestones.

## Priority order

Set by the owner on 2026-11-10:

1. **MM7 working.** Done 2026-11-11.
2. **MM6 working**, while keeping MM7 working. Active.
3. **Full MM7 audit.** In progress in parallel under the MM7 owner.
4. **Full MM6 audit.**

MM8 and cross-game transfer follow these four. Mac polish, packaging and public release work do not block them.

## Ownership

Since 2026-11-12 title authority is strict:

- **Codex** leads the project and is the sole owner of MM7: its working gate, audit, defects, lane assignment and
  integration.
- **Claude** owns all MM6 work on the same terms.
- Neither owner changes or accepts the other title's work without an explicit owner instruction. A change to shared
  engine code runs the other title's regression controls before it merges.

`master` is the neutral integration branch. Since 2026-11-18 finished MM6 lanes merge into the long-lived
`claude/mm6` branch until the MM6 release tag. Branches use `codex/` and `claude/` prefixes. Worktree budgets, claims,
the live mailbox and the merge procedure are in [Agent workflow](agent-workflow.md).

## Repository layout

| Path | What it holds |
| --- | --- |
| `src/` | The engine. `Application` holds startup, the game loop and config. `Engine` holds rules, objects, events, graphics and saves. |
| `src/Engine/MM6`, `src/Engine/MM7` | Title-owned engine code. MM6 code also lives in `MM6*` files next to the shared code it hooks. |
| `src/GUI`, `src/Media`, `src/Io`, `src/Library`, `src/Utility` | Screens, houses and dialogue, audio and video, input and file I/O, platform libraries, utilities |
| `src/Bin` | The `OpenEnroth`, `MM7` and `MM6` executables, `LodTool`, `CodeGen` |
| `test/Bin` | `UnitTest`, `GameTest` (input-driven game tests against real data, `GameTests_MM6*.cpp` for MM6), `RetraceTest` |
| `test/Testing`, `test/Data` | Shared test helpers, upstream MM7 trace and save fixtures |
| `tools/` | Mac build wrappers, the MM6 first-map gate, map and combat sweeps, the campaign chain runner, the MM7 gameplay harness, the playtest capture script, the worktree budget check |
| `launcher/` | The Atlas: Swift app, HTML interface, bundle assembly and its tests |
| `resources/` | Shaders, scripts, data and images that ship with the engine |
| `docs/` | This documentation |
| `local-game-data/` | Git-ignored home for the owner's game installs, captures and private research. Only its README is tracked. |
| `agent_comms/live/` | Git-ignored live mailbox between agents on the owner's Mac. Only its README is tracked. |
| `thirdparty/`, `CMakeModules/`, `android/`, `distribution/`, `scripts/` | Inherited from upstream: dependencies, CMake helpers, platform packaging, style scripts |

`HACKING.md` holds the build, code style and test rules inherited from upstream and extended by the fork.

## Source

- [README.md](../README.md), [CREDITS.md](../CREDITS.md), [HACKING.md](../HACKING.md)
- [GameTitle.h](../src/Engine/GameTitle.h)
- [src/Bin/OpenEnroth/CMakeLists.txt](../src/Bin/OpenEnroth/CMakeLists.txt), the `MM6` and `MM7` app targets
- [GameBuild.swift](../launcher/GameBuild.swift), helper arguments and profile folders
- [PathResolver.cpp](../src/Application/Startup/PathResolver.cpp)

See also: [MM6 campaign status](mm6-campaign-status.md), [MM7 status](mm7-status.md), [Releases](releases.md), [Agent workflow](agent-workflow.md), [Upstream](upstream.md)
