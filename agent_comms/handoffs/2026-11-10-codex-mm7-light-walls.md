# MM7 Light city and Walls of Mist handoff — 2026-11-10

- Branch: `codex/mm7-light-walls`
- Claim: `e5cc789a2`
- Code: `55028de82`
- Files owned: new `test/Bin/GameTest/GameTests_MM7LightWalls.cpp` and its `test/Bin/GameTest/CMakeLists.txt` registration.

The `LightRoute` test uses the judge and arbiter inputs to choose Light, enters Celeste through the Bracada portal trigger, enters Castle Lambent and accepts the Walls quest through its daytime throne NPC, enters Walls of Mist, uses all three pedestal keys through face clicks, exits back to Celeste, then returns to the throne and hands in the quest. It tapes Light choice, active/passed bits, guild membership and map transitions. The Walls pass bit changes when the exit interaction opens the transition prompt, before the player accepts travel; the test asserts that timing.

`ThroneClosedAtDawn` waits to 5am with a Light-path fixture and verifies the same Lambent throne door stays closed. The positive test proves that it opens at the starting daytime hour. The hour gate is supplied by the local house table and evaluated by the normal house UI.

Checks under headless-only authorization:

- `OpenEnroth_GameTest` build passed.
- `MM7LightWalls.*` passed 2/2.
- Combined `MM7LightWalls.*:MM7Campaign.ArbiterChoice:Issues.Issue2463a` passed 4/4.
- `check_style_OpenEnroth_GameTest` and `git diff --check` passed.
- Removing the final throne hand-in input caused the expected `LightRoute` failure at active/guild assertions; restoring it returned the focused pair to 2/2.

The protected-group-kill negative branch remains **open**. The Walls exit script checks for one killed actor in group 1, but the local runtime loads no group-1 actor, both before and after the three pedestal actions; all currently loaded actors are in a different group. The local map program showed no group-1 summon or group-change action. This test does not inject an actor and does not claim to validate a native protected-group kill. Determine whether this is a shipped-map/source mismatch or a runtime actor-group translation issue before designing that branch. Do not infer a player-facing bug from this probe alone.

A fresh-process Save/Load checkpoint after this chapter is a separate W4 gate. Only headless checks ran. No private data, raw scripts, saves, inventories, coordinates beyond existing public test anchors, or logs were committed. The main checkout was not edited, merged or pushed.

Focused rerun:

```sh
build/test/Bin/GameTest/OpenEnroth_GameTest --game mm7 --headless --data-path "$MM7_DATA_PATH" --test-path test/Data '--gtest_filter=MM7LightWalls.*' --gtest_brief=1 --log-level=error
```
