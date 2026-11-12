# MM6 house exits and the NPC 12 room chain handoff (M4 lane X, done)

- Owner: Claude Code lane. Branch `claude/mm6-house-exits`, rebased on `master` `6114f76e8`. Not pushed.
- Research: `2026-11-11-claude-mm6-house-exits.md` on the same branch. Its claims were rechecked against MM6.exe and the shipped data before building on them. One correction: outc2:153 also names house 170, but as the house of a MoveToMap into the Oracle map, not a SpeakInHouse that runs.

## Commits

- `430f734d0` WIP research handoff (rebased from `e98e66fe0`).
- `fb022a4bc` Offer MM6 house exits and run the King's Library room chain.
- This handoff, on top.

## What works

- House table columns 20-22 are kept in `mm6::HouseServiceRow::exit`, read with the original's atoi semantics.
- A house lists its exit as the last portrait, labelled "Enter <map>", with the icon from the original's eight-entry exit picture table. A positive restriction is a quest bit that must be set. At most six portraits show, as in the original.
- Clicking the exit opens the confirmation view: Cancel and Yes on the y = 313 row (`buttesc2` and `buttyes2`), the portrait and the room as Yes, the map name as the title and trans.txt row `map` or "Enter <map>" as the text. Keys N, Y and Space.
- Yes calls `startMapTransition`, which parks the map and autosaves, plays sound 75 and leaves the house. A negative restriction picks the arrival from the original's six-entry table, otherwise the map's party start.
- Room chain: with Tanir's Bell (item 461) on a character, the King's Library (168) plays `archie` once, then the party is in 553 with Archibald. MoveNPC while in 553 lets `archloop` finish its pass, then the party is in 554. The chain runs on a platform-tick clock with the length read from the Smacker header, so it is deterministic in tests and needs no playback.

## Files

- New: `src/Engine/MM6/MM6HouseExit.h`, `src/Engine/MM6/MM6RoomMovie.h`, `src/Engine/MM6/Tests/MM6HouseExit_ut.cpp`, `test/Bin/GameTest/GameTests_MM6HouseExit.cpp`.
- MM6-owned: `src/GUI/UI/Houses/MM6House.{h,cpp}`, `src/GUI/UI/MM6HouseServiceData.{h,cpp}`.
- Shared-core hooks, MM6-gated, MM7 paths unchanged: `src/Application/Game.cpp` (the `UIMSG_HouseTransitionConfirmation` case and one call after `HouseMovieLoop`), `src/Engine/Evt/EvtInterpreter.cpp` (one call after the MM6 MoveNPC).
- `src/Media/MediaPlayer.{h,cpp}`: `houseMoviePayload`, an accessor next to `hasHouseMoviePayload`.
- CMake: `src/Engine/MM6/CMakeLists.txt`, `test/Bin/GameTest/CMakeLists.txt`.

## Checks

- `check_style`: clean. `check_tidy`: not run, no clang-tidy on this machine and the build has no `check_tidy` target.
- `OpenEnroth_UnitTest`: 1167 ran, 1122 passed, 45 skipped, 0 failed. Includes the two new unit tests.
- MM6 suite `MM6*`: 121 ran, 119 passed, 2 skipped (`MM6ZSaveHeadless` private-research tests), 0 failed, 93 disabled.
- MM7 controls `MM7*:VendorRefreshIntegration.*`: 109 ran, 107 passed, 2 skipped (`VendorRefreshIntegration` buy-tab tests), 0 failed.
- The four `MM6HouseExit` game tests fail with `Game.cpp`, `EvtInterpreter.cpp` and `MM6House.{h,cpp}` reverted, and pass with them.
- Not run: `Run_GameTest_Headless_Parallel` (lead runs it at integration). No progress capture: `enroth-capture` renders a map's entrance view, and the visible change is inside a house screen.

## Open

- House 165 is also in the original's room watcher. MoveNPC while in the High Council (the traitor step, global:380) re-enters 165 with the one-shot `Citytrtr` movie, and when it ends 165 is entered again, which drops Slicker's portrait. Not built here. The traitor step still works, his portrait stays until the party re-enters.
- House 168 and 554 have no people, so with video off (`no_video`, the headless test default) `mm6EnterHouse` refuses them as rooms that show nothing. The tests turn video on. With video on, movie frames play on wall-clock time while the chain runs on platform ticks, which are the same clock outside tests.
- The original autosaves on Yes even for an exit to the current map. `startMapTransition` autosaves only for another map. No shipped exit leads to its own map.

Privacy: addresses, IDs and map stems only.
