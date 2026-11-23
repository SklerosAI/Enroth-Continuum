# Source and privacy

This page draws the line between the public source in this repository and the private game data it runs on.

## The boundary

This is a public repository. It holds the engine source, tests, tools, the launcher and these docs. The Might & Magic
VI, VII and VIII game data is proprietary and stays on the owner's machine. Every player supplies their own lawfully
obtained installation, and nothing in Git or in a release redistributes it.

The engine reads what it needs from the player's own files at runtime. The MM6 rules are an example: class creation
arrays, spell costs, house rooms, transport routes and shop stock come out of the player's `MM6.exe` through
`mm6::Executable`, not from a table copied into the repository.

## What never goes in Git

None of these go into a commit, an issue, a pull request, a release, a review note or the live agent mailbox.

| Kind | Examples |
| --- | --- |
| Installers and archives | GOG installers, `.lod` archives, the shipped soundtrack and manual archives |
| Extracted game files | Maps, event scripts, text tables, sprites, textures, music, sound, video |
| Guide and manual text | Strategy-guide or manual passages, quoted or paraphrased closely |
| Saves | Original saves, `.ec6` and `.mm7` saves made from the real data, save backups |
| Logs and traces | Local run logs, command output dumps, research reports |
| Screenshots and video | Captures of the running game, hidden offscreen captures, recordings |
| Private mods | The owner's portrait and armor overrides and other local mod files |
| Inventories | Per-record or per-asset tables decoded from the data, checksums of private files |

`test/Data/` holds only the trace and save fixtures that came with upstream OpenEnroth. New tests start a new game and
build their state in code, and no new trace or save goes into `test/Data/`.

## What derived data is allowed

Describe formats and behaviour in your own words. These are fine in source, tests, commit messages and docs:

- Binary layouts: record sizes, field offsets, field names we chose, flag bits.
- Addresses in the original executable, such as the virtual address of a branch in `MM6.exe`, with what the code
  there does.
- Original-game behaviour stated as a rule, with where the evidence lives.
- The individual identifiers a fix or a test needs: a map file name, an event number, an item or monster id.
- Aggregate results, such as how many maps have a given face type, without the per-record list.

A full table of every record, a decoded string list, or a per-map asset catalogue is an inventory, even when it's
written as Markdown. Keep it under `local-game-data/`.

## local-game-data/

`local-game-data/` is the private vault in each checkout. Git ignores everything in it except its
[README](../local-game-data/README.md), through the `/local-game-data/*` rule in [.gitignore](../.gitignore). Never
use `git add -f` on anything inside it.

| Folder | Holds |
| --- | --- |
| `mm6/`, `mm7/`, `mm8/` | Installers, extracted installations, research outputs, sweep results and per-task captures for each title |
| `progress-captures/` | Rolling hidden captures taken while agents work, capped at 100 files |
| `private-wiki/` | The private companion wiki, see below |
| `public-references/` | Pinned copies of public third-party sources, such as the GrayFace patch, and data decoded from them |

The main checkout's `local-game-data/` is the canonical one. Linked worktrees have their own empty copy, so tools and
agents in a worktree point at the main checkout's folder through environment variables or explicit paths.

Per-task MM6 captures go under `local-game-data/mm6/task-captures-<date>/<lane>/`. Tools that write research output,
such as `tools/run_mm6_map_sweep.py` and `tools/run_mm6_combat_sweep.py`, default to a dated folder under
`local-game-data/mm6/`. `tools/mm7_playtest_capture.sh` refuses a destination in a tracked part of the repository.

## How tests and tools find the data

Nothing hard-codes a private path. The game, tests and tools take the data folder from the environment or the
command line.

| Consumer | How it finds the data |
| --- | --- |
| The game | `OPENENROTH_MM6_PATH`, `OPENENROTH_MM7_PATH` or `OPENENROTH_MM8_PATH`. On macOS `ENROTH_CONTINUUM_MM7_PATH` wins for MM7. |
| The game, no override | The current folder and a per-title folder under `~/Library/Application Support/` |
| Unit tests | `MM6TestData` opens the folder in `OPENENROTH_MM6_PATH`. A test that needs it skips when the variable is unset, which is the CI case. |
| Game tests | `OpenEnroth_GameTest --game mm6 --data-path <folder>`, or the same environment variables. |
| Gate and sweep tools | `MM6_DATA_PATH` and `MM7_DATA_PATH`, as `tools/run_mm6_firstmap_gate.py`, `tools/run_mm6_combat_sweep.py` and `tools/run_mm7_campaign_chain.py` read them. |

The [testing page](testing.md) has the full option list. CI has no game data, so a data-backed test proves nothing
until someone runs it locally against all the titles it touches.

## Captures and recordings

Captures are the main visual evidence for headless work, and they show original game art, so they are private. They
stay under `local-game-data/`, never in Git, an issue or a pull request. A pull request or commit message describes
what a capture shows in words. Anything that accumulates, captures, recordings or save snapshots, needs a cap.

## History and publishing

Git history is permanent. Redacting a file in a later commit doesn't remove what an earlier commit published. Before
pushing a branch, check its current tree and every new commit on it, not just the final diff. Review
`git status --short` and the staged paths before every commit, and run `git check-ignore -v <path>` when unsure
whether a private file is covered.

## The private companion wiki

`local-game-data/private-wiki/` is an owner-local knowledge base with an `INDEX.md` and `mm6/`, `mm7/` and `mm8/`
folders. It holds what can't go into public Git: exact per-record findings, field interpretations, discrepancies
between records, and pointers to local reports, captures and traces.

The `local-game-data/*` rule covers it, so Git never backs it up. The owner keeps a separate local backup. A clean
public worktree says nothing about whether these files exist or can be recovered.

A private entry records where the finding came from (the owned file, the method, the source revision) and how far it
got: observed bytes, a parse, an inferred meaning, runtime behaviour, or campaign play. Public docs may point to a
private page by name and give the aggregate conclusion, but never copy its rows, strings or images.

## Source

- [.gitignore](../.gitignore)
- [local-game-data/README.md](../local-game-data/README.md)
- [CREDITS.md](../CREDITS.md)
- [src/Engine/MM6/MM6Executable.h](../src/Engine/MM6/MM6Executable.h)
- [src/Application/Startup/PathResolver.h](../src/Application/Startup/PathResolver.h)
- [test/Testing/Unit/MM6TestData.h](../test/Testing/Unit/MM6TestData.h)
- [test/Bin/GameTest/GameTestOptions.cpp](../test/Bin/GameTest/GameTestOptions.cpp)
- [tools/run_mm6_firstmap_gate.py](../tools/run_mm6_firstmap_gate.py)
- [tools/mm7_playtest_capture.sh](../tools/mm7_playtest_capture.sh)

See also: [Upstream relationship](upstream.md), [Agent workflow](agent-workflow.md), [Testing](testing.md),
[MM6 executable rules](mm6-executable-rules.md), [MM6 GrayFace patch reference](mm6-grayface-patch-reference.md)
