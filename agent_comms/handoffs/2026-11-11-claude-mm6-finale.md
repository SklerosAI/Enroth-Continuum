# MM6 hive finale handoff

- Owner: Claude Code lane F. Branch `claude/mm6-finale` in the `mm7-save-durability` worktree, rebased on `master` `6114f76e8`. Not pushed.
- Research behind it: `2026-11-11-claude-mm6-finale-wip.md` on this branch. This round re-read every address it relies on in GOG MM6.exe. One correction: the reactor summons 16 monsters, not 15 (16 calls to the summon routine from `0x403258` to `0x403438`).

## What works

The MM6 ending runs as the original does and never enters an MM7 map.

- Blasters hurt the hive reactor. MM6 blasters launched MM7's object 555, which MM6 has no row for, so no shot ever flew. The original's spell-to-object table (file offset `0xbdbd4`, entry 102) launches object 590, the laser bolt. Its damage type in the party damage routine (`0x4315b0`) is 6, Energy, which the monster resistance routine (`0x421dc0`) resists with nothing. The shot now flies as 590, deals Energy and turns into 561 on impact, the explosion the monster laser uses. The party side of the original's impact code for 590 was not read.
- The Demon Queen's death sets quest bit 202. The reactor's death without the Ritual of the Void (item 544) loses the game at once. With it, quest bit 180 is set, 16 guards are summoned, doors 28, 30, 51, 52 and 53 move, the party is placed at the hive exit (3328, 25920, yaw 512) and every condition is cleared.
- A map load removes the queen and the reactor once their bits are set.
- Houses 600 and 601 play the winning and the losing ending: movies `comped`, then `end_dome` and `end_seq1` or `planetxp`, then the win screen (party, play time, score, saved as `MM6_Win.Pcx`) or the losing screen. The screen ignores keys for 5 seconds. A won game then shows the farewell box (globals 151, 118, 167) and goes on at New Sorpigal's party start without an autosave. A lost game quits to the main menu.
- While the hive finale is underway (hive, bit 180 set, bit 237 clear), party death and any map change play the losing ending, and autosave is skipped.

## Commits

1. `58cfeff68` `Fire MM6 blasters as the laser bolt, with Energy damage` - `CastSpellInfo.cpp`, `SpriteObject.cpp`, `Actor.cpp`, `MM6MonsterDamage.h` and its unit test, game test `MM6Finale.BlasterHurtsReactor`. Its intermediate test file was not built on its own, the final tree was.
2. `02ea5e70a` `Run the MM6 hive finale and its endings` - `src/Engine/MM6/MM6Finale.{h,cpp}`, MM6 window in `UIGameOver.{h,cpp}`, MM6 screens in `GameOver.{h,cpp}`, hooks in `Game.cpp` (show, close, change location), `UIHouses.cpp` (`enterHouse`), `Actor.cpp` (`Actor::Die`), `Engine.cpp` (map load), `MM6PartyDeath.cpp` (hive branch), `MM6SaveLoadMenu.cpp` (autosave). Tests in `test/Bin/GameTest/GameTests_MM6Finale.cpp`.

## Tests and checks

New suite `MM6Finale`, 6 tests, all from production New Game through input: `BlasterHurtsReactor`, `WinsWithRitual`, `ReactorWithoutRitualLoses`, `ObeliskWithoutRitualLoses`, `DeathInHiveLoses`, `LeavingHiveLoses`. With the finale hooks reverted the five finale tests fail and the blaster test passes. With only the blaster fix reverted the blaster test fails.

Checks on the final tree, one process at a time: `check_style` clean. `check_tidy` not run, this build has no clang-tidy and no such target. `OpenEnroth_UnitTest` 1120 passed (two MM6 damage gate tests updated, Energy is now a verified monster channel). MM6 suite `MM6*` 121 passed, 2 skipped (`MM6ZSaveHeadless`, need private research), 93 disabled. MM7 controls `MM7*:VendorRefreshIntegration.*` 107 passed, 2 skipped (the two vendor refresh tests), 20 disabled. Capture of the hive entrance: `local-game-data/progress-captures/20261111-140654-mm6-hive-finale-hive.png`.

Fixtures: the hive is entered by `startMapTransition` with the outa1:90 placement (the hive has random chests, so a bare teleport refuses). Every hive monster but the two bosses is set `Removed`, the bosses are set to 1 HP before the blaster fight, the first character gets a blaster and Blaster skill, and the Ritual is put in a pack (Archibald's `global:30` gives it in play). The death test spawns four monsters 108 around the party.

## Open

- In a probe none of the 16 summoned guards hurt the party in the 1000 frames after the reactor blew. Whether the original's guards attack at once is not checked.
- MM6 bows have the same object problem as blasters: the original table gives the arrow object 500 for spell 100, the engine launches MM7's 545. Not touched here.
- Autosave skipping in the hive has no test. MM6 tests have no profile root, so autosave never writes in them.
- The farewell box uses MM7's box rectangle. The original opens a full-screen window of type 70 and its draw code was not read. House 602 has no caller in the exe or the data and is not handled. The flags bit at `0x6107d8` that skips the first two movies is unknown and ignored.
- The ending takes Escape and a left click, the original takes any key.

Privacy: addresses, IDs and map stems only.
