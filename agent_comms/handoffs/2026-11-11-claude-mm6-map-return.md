# MM6 map state across a return trip handoff (M2 part 1c)

- Owner: Claude Code lane, reviewed and integrated by the lead.
- Branch: `claude/mm6-map-return`, commits `45ce7e223`, `c1db9506b` and `e5b12576f`, cherry-picked onto `master` as `62217fdec`, `c22a79731` and `2e079b015`.
- Hooks, one line each and MM6-gated: `Outdoor.cpp`, `Indoor.cpp`, `Game.cpp` `gameLoop`. Everything else is in `MM6MapTransition`. `Chest::OnChestLeftClick` gains an MM6 branch.

## Behavior

Leaving a map parks it in session memory: the live sidecar (reset counters, visibility, chests and, indoors, doors), plus the actors, ground items and event variables. Returning reads the parked delta instead of the session-start one. If the map didn't respawn, it restores the parked actors, ground items and variables. A new or loaded game forgets every parked map, and nothing reaches disk. The restore runs inside Load before the chest pass and street-NPC generation, matching the original's order at 0x456740: one chest-pass RNG draw on a return, and street NPCs regenerated.

The new-game chest seed now comes from `grng` instead of `std::random_device`. MM6 chests now let a click take a non-gold item to the cursor, updating the chest and its sidecar together.

## Checks

`MM6MapTravel.DungeonAndBackKeepsTheMap` and `NeighbourAndBackKeepsTheMap`, input-driven: kill a monster, loot a chest, go through a dungeon door or over an edge, come back, and both are still so. Revert-and-fail without the store and without the pickup. On the branch: unit tests 1,105, MM6 suite 100, first-map gate 27 with the MM7 controls, MM7 in-process 107, full MM7 540/540, MM6 sweep unchanged, `check_style` clean.

## Open

Saving maps other than the first (S-SAVEANY, active). Taking gold from chests and putting items back. Outdoor faces and decorations are not parked, and the original's behavior there is not traced. Transport, Town Portal and Lloyd's need `leaveMap` once MM6 can use them.

Privacy: addresses, IDs and map stems only.
