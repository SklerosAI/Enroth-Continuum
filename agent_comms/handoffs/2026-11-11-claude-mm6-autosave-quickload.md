# MM6 autosave, quick save and in-game Load handoff (M2 and M3 remainder)

- Owner: Claude Code lane, reviewed and integrated by the lead.
- Branch: `claude/mm6-autosave-quickload`, commits `2d56e3da6`, `46047a125`, `fc90810b4`, on `master` as `2a9f31413`, `a0e557dff`, `657878857`.
- Hooks, one line each and MM6-gated: `Game.cpp` (new-game autosave), `GameMenu.cpp` (Load), `UISaveLoad.cpp` (Save list filter). `SaveLoad.cpp` routes its MM6 branches to MM6 functions.

## Original behavior (MM6 executable)

The save routine's autosave flag copies the session save over the autosave file. It autosaves after New Game, on confirming a map-edge crossing (before the trip's days pass), on MoveToMap events and the dungeon-door dialog to another map, on stable and boat routes to another map, on Town Portal, Lloyd's Beacon and house transitions, and on the quit prompt's Yes. It never saves in the arena, and doesn't autosave in the hive during the reactor state. There are 40 numbered slots plus the autosave, which the Load list shows first and the Save list hides. There is no quick save or quick load in the original. Escape-menu Load runs the same slot load as the main menu and replaces the whole session.

## Design

The slot policy admits `saveNNN.ec6`, `autosave.ec6` and `quicksave0-9.ec6`, all with atomic writes, backup rotation and refusal of foreign files. `autoSave()` for MM6 writes the autosave after the map is parked. In-game Load stages and checks the slot before changing anything, then drops the running session's parked maps and anchor and loads as the main menu does. A slot that can't load keeps the game. F5 and F9 are an engine addition, matching MM7's rotation.

## Checks

Gate children `MM6AutosaveHeadless.EdgeAndDoorWriteTheAutosave`, `AutosaveLoadsAndQuickSaveComesBack` and `QuickSaveLoadsInFreshProcess`. Normal-suite `MM6InGameLoad.EscapeMenuLoadReplacesTheSession`. Revert-and-fail for each piece. On the branch: MM6 suite 115, MM6 first-map gate 32 with the MM7 controls, MM7 in-process 107, full MM7 540/540, unit tests 1,120, `check_style` clean.

## Open

The arena and hive save rules and the quit-prompt autosave are not implemented. Main-menu F9 refuses for MM6. Each autosave runs the full capture and validation, and its cost in visible play is unmeasured. A load that keeps the old parked maps stalls at the loading screen for an unknown reason. The shipped path drops them first, but the cause should be found. Project docs still describe these as guarded.

Privacy: addresses, IDs and map stems only.
