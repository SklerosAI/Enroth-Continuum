# MM7 Light quest-item acquisition handoff

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: the Claude integration lane.
- Branch: `claude/mm7-light-items` in the `mm7-save-durability` checkout, commits `88cbca0f5` (WIP) and `d627376af` (completes it). Not pushed.
- File: new `test/Bin/GameTest/GameTests_MM7LightItems.cpp` and its CMake line.

## Tests

- `WallsPillarKeysFromChests`: the three pillar keys from Walls of Mist chests, then the pedestals and the exit doors, looted versus not looted. One chest sits under a floor door that opens only after the lever puzzle, which the test solves by input.
- `AltarPiecesFromTempleChests`: the Light piece from the Temple of the Light and the Dark piece from under the Temple of the Dark altar lid. The hand-in is refused with one piece and accepted with both.
- `SoulJarsFromGloamingChest`: refused without the case, accepted after looting Castle Gloaming.
- `WineCellarVampireQuest`: take the quest, enter, leave through the map exit, hand in. The exit sets the vampire bit in the shipped data, and the named vampire loads disabled, so there is no fight.

Chests are opened by aiming at their face with `pointMouseAtFace` and clicking, and trapped chests take a second click. One party-toughness fixture is kept, because without it the party dies in the Walls of Mist. No NoDamage or NoActors.

## Checks

- `MM7LightItems.*` 4/4. With controls `MM7LightAdvisors.*`, `MM7LightWalls.*`, `MM7OverthrusterRoute.*`, `MM7PitCubeAcquisition.*` and `Issues.Issue2463*`: 16/16.
- Behavior guards: skipping the Temple of the Dark chest or the Gloaming loot fails the hand-in. Skipping the cellar visit leaves the bit clear.
- `check_style` passes. Full headless MM7 binary 537 passed, 2 skipped, 0 failed.

## Open

The Wine Cellar's named vampire (actor 0, `MONSTER_VAMPIRE_C`) loads with AI state `Disabled` and attribute `0x10000` (`ACTOR_UNKNOW11`, undocumented upstream). Whether the original game ever activates it is unverified. Stage 3 audit item.

Privacy: no game data, saves or logs in Git.
