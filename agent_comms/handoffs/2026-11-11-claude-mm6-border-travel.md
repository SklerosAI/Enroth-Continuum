# MM6 live travel handoff (M2 part 1)

- Owner: Claude Code lane, reviewed and integrated by the lead.
- Branch: `claude/mm6-border-travel`, commits `ad8565494` and `038acccb6`, cherry-picked onto `master` as `adfc6dc45` and `86bff5ad3`.
- Shared core, MM6-gated: `Outdoor.cpp` (travel destination, edge offer flags, arrival hook), `Game.cpp` (`UIMSG_OnTravelByFoot`), `Engine.cpp` (`startMapTransition` calls `leaveMap`), `SaveLoad.cpp` (`autoSave()` logs and returns for MM6). New `src/Engine/MM6/MM6MapTransition.{h,cpp}`.

## Original behavior (MM6 executable)

There is no border table. The neighbour comes from the current map name's column letter and row digit on a 5 by 3 grid, crossing at ±22528, with corners allowed and off-grid refused. The offer is refused while the party is in water, airborne or on water. Travel takes 5 days minus guide-type hirelings, minimum 1. Confirming advances time, rests once with food, makes everyone Weak and adds a day if food is short, eats the food, wraps the crossed axis by 45056 and sets Z from the new terrain. Cancel clamps the party back inside the edge. Addresses are in the commit messages.

## Behavior now

Walking off an outdoor edge offers the neighbouring map and applies all of the above. Dungeon doors and their exits work both ways through the MM6 transition dialog, now with a Yes button. A dungeon load resets stale chest positions, as the outdoor load already did, so it no longer refuses its chests. MM6 autosave on a transition is skipped with a log line.

## Checks

`MM6MapTravel.WalkOffEdgeToNeighbour` and `MM6MapTravel.DungeonDoorAndBack`, input-driven from New Game with positions, time and food pinned, plus revert-and-fail. MM6 suite 98 passed. MM6 first-map gate 27 with the MM7 controls. MM7 in-process 107. Full headless MM7 540/540. MM6 sweep unchanged (67 loaded, 66 finished). Unit tests 1,105 passed. `check_style` clean.

## Open

- Returning to a map reloads its session-start state, so monsters respawn and chests refill. The same lane continues on `claude/mm6-map-return` with an in-memory per-map runtime store.
- Leaving the first map disarms manual save for the session. Saving on other maps is a later M2 scope.
- Transport, Town Portal and Lloyd's don't call `leaveMap` yet. The travel dialog uses MM7's layout and art.
- The detached `MM6OutdoorBorderPrecommit.h` and `MM6OutdoorReturnArrival.h` still refuse corners, in test-only code.

Privacy: addresses, IDs and map stems only.
