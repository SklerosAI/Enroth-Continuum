# MM6 save and load on any map handoff (M2 part 2)

- Owner: Claude Code lane, reviewed and integrated by the lead.
- Branch: `claude/mm6-save-any-map`, rebased onto `master` `7f279f9dc`, commits `0da9d0e0a`, `d436e8d82` and `3b27416f7`, cherry-picked onto `master` as `5e752feab`, `583875015` and `0614e727b`. One test CMake conflict next to the party-death test was resolved by keeping both lines.
- Hooks, MM6-gated: `Game.cpp` (a failed MM6 main-menu load returns to the main menu), `SaveLoad.cpp` (autosave log text and the quick-save refusal), `MM6SaveLoadMenu.cpp` (the refusal message). New `MM6SessionSave.{h,cpp}` and `MM6ParkedMapSection.{h,cpp}`.

## Design

Loading a save works like returning to a parked map in the same session. While the party has never left the first map, the existing v5 path runs. After that, saving builds an EC6 v7 save: the 19 raw v2 owner sections captured from the live party, then a required `PRKM` section with every visited map's live lists (event variables, exact actor images and fields, ground items), and each visited map's native delta in MAPS as the map-return store parks it. Loading stages the raw owners through the existing checks, hands every map to the store and requests the active map, and the existing parked-delta restore rebuilds it with the chest pass and street NPCs as on a return. Generated NPCs and the RNG state are not saved. A numbered slot holds a v5 or a v7 save, and either can replace the other with backup rotation.

## Format

EC6 container `MAX_VERSION` 6 to 7. PRKM version 1, with an exact round trip on encode and a canonical-form check on decode. Other container or PRKM versions, trailing bytes, missing or extra sections and foreign versions are refused. v1 to v6 are unchanged.

## Checks

Gate children `DISABLED_DungeonSaveAfterTravel` and `DISABLED_DungeonSlotLoadsWithItsOutdoorMaps`: New Game, loot and kill on the first map, cross to `outd3` and kill there, enter `d06` and save. A fresh process loads it from the Load menu with position, stats and bags matching, and on walking out both outdoor maps are still as left. Saving again rotates the backup. Revert-and-fail. On the branch: MM6 suite 100, MM6 first-map gate 29 with the MM7 controls, MM7 in-process 107, full MM7 540/540, sweep unchanged, unit tests 1,115, `check_style` clean.

## Open

Autosave and quick save do not write yet, because they need non-numbered slot names in the policy. Outdoor faces and decorations are not carried. In-game Load is still refused (M3). MM6 chest traps are not implemented, so trapped chests never open (new lane). No project doc pins EC6 v7 yet.

Privacy: IDs, section tags and map stems only.
