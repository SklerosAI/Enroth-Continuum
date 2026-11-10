# MM7 Light chapter fresh-process checkpoint handoff

- Owner: Codex lane, branch `codex/mm7-light-checkpoint` from integration master `e52c1246c`.
- Integration: pending parent review. No merge, push, graphical run, wiki or PLAN edit.

## Scope

`GameTestMain.cpp` admits two exact disabled Light checkpoint filters through the existing marked, owner-only disposable MM7 profile gate. The writer reuses the input-driven Light route through Walls of Mist and the throne-room hand-in, confirms its assertions and final Light, Walls, and guild state, and writes `save000.mm7` through production `saveGame`. The fresh-process reader selects that slot through the normal Load menu and checks the resulting map and quest/guild state. It compares slot bytes before and after Load.

An exploratory assertion on the throne NPC's dialogue event ID was removed after focused evidence showed that this transient field is reset on fresh Load. Persisted quest and guild bits are the checkpoint contract.

## Focused results

- `OpenEnroth_GameTest` built and linked.
- `MM7LightWalls.LightRoute:MM7LightWalls.ThroneClosedAtDawn` passed 2/2.
- In a fresh marked profile, disabled writer and separate-process reader passed 1/1 each; save-slot bytes were identical before and after Load.
- A non-checkpoint filter paired with the disposable profile was rejected before game startup.
- Changed-file `cpplint.py` and `git diff --check` passed.

## Reproduction boundary

Create a fresh owner-only `mm7-save-gate-*` directory directly under the system temp directory with only marker `.ec-mm7-disposable-save-gate` containing `EnrothContinuum-MM7-save-gate-v1`. Supply local MM7 data, `--game mm7 --headless --test-path test/Data --mm7-disposable-profile`, and `--gtest_also_run_disabled_tests`. In separate processes, run exact filters `MM7LightWalls.DISABLED_WriteFreshCheckpoint` then `MM7LightWalls.DISABLED_ReloadFreshCheckpoint`. Keep the profile, saves, game data, raw logs, and checksums out of Git.
