# MM6 hive finale handoff (WIP, research only)

- Owner: Claude Code lane. Branch `claude/mm6-finale` in `mm7-save-durability`, from `master` `731e7a0a8`.
- State: the original behavior is established from MM6.exe and the shipped scripts. No code is written and nothing is tested.

## Original behavior (GOG MM6.exe, objdump of `.text`)

Monster death, `0x403050` (the counterpart of `Actor::Die`):
- Monster 172 dies: quest bit 202 is set (`0x4031bb`) and UI sound 997 plays.
- Monster 173 (the reactor) dies: if no character carries item 544 (`0x4031f8`, the var 17 compare per character), `EnterHouse(601)` runs at once (`0x40321e`). Otherwise quest bit 180 is set (`0x40322e`), 15 monsters are summoned through the SummonMonsters routine `0x43ea00` (encounter slot 1 or 2, tier 3, count 1, z -2256, at fixed x/y pairs listed at `0x403247`..`0x403438`), doors 28, 30, 51, 52, 53 get actions 0, 1, 0, 0, 1 (`0x43fc60`), the party moves to x 3328, y 25920 with yaw 512 and z kept, UI sound 232 plays, and every character's conditions are cleared (144 bytes at character offset 4992).
- The reactor takes only Energy damage, its monster table row is immune to the other six schools. That is the blaster requirement, there is no engine check for it.

Map load: on every load, outdoor `0x46e8c9` and indoor `0x455748`, monster 173 is removed while bit 180 is set and monster 172 while bit 202 is set (`0x44c050`). `hive:100` (OnMapReload) replays the same five door actions while bit 180 is set.

Scripts (all 83 MM6 `.evt` files scanned): `hive:60` is the exit. It needs bit 202, then checks item 544. With it, bit 237 is set, bit 198 cleared, item 544 removed, award 36 given, house 600. Without it, house 601. `outa1:90` refuses the hive entrance once bit 237 is set. `global:30` gives item 544. No script uses house 602. The hive timers (`hive:51`, `54`, `57`) are a flush trap on map variables, there is no reactor countdown.

`EnterHouse` `0x43c300`: house 600 calls the ending `0x4a6cb0` with 0, 601 with 1, 602 with 2.

Ending `0x4a6cb0(kind)`:
- Movies from Anims2: `comped`, then `end_dome` for kind 0 or `planetxp` for kinds 1 and 2, and `end_seq1` for kind 0 (`0x4a6d71`..`0x4a6da6`, through `0x4a59a0`).
- Kind 0 draws `winbg.pcx` with `endgame.fnt` in a 75, 60, 495 by 338 frame: global.txt 9 at y 35 centred, one line per character from global 129 (name, level, class) at y `i*(h-2)+h+46`, the `win.str` text at y `5*(h+7)`, global 37 plus years, months (28 days) and days with globals 245/132, 146/148, 56/57 at y `338-2h-5`, and global 94 with score = total experience / days played (at least 1) at y 338. The screen is saved as `MM6_Win.Pcx`.
- Kind 1 draws `losebg.pcx`, global 9 at y 35 and the `lose.str` text at y 110. Kind 2 draws nothing.
- Kinds 0 and 1 then wait at least 5 seconds and for a key (`0x4a730a`..`0x4a73f0`), kind 2 doesn't wait.
- Kind 0 posts message 197, kinds 1 and 2 post 132 with parameter 1 (`0x4a7414`..`0x4a746e`).

Messages: 132 with parameter 1 (`0x42dcc4`) skips the quit confirmation and its sound and sets game state 7, quit to the main menu. 197 (`0x42f142`) opens a full-screen box with globals 151, 118 and 167 (`0x40f480`, release message 196) and sets game state 9. Escape or a click closes it (`0x40f4d0`, called from `0x42b809` and `0x43023f`) and posts 196. 196 (`0x42f186`) parses the constant "40" as entry 40 of the Games.lod directory, which is `oute3.odm`, and sets game state 2. The party lands on the Party Start decoration with start index 0, the index MoveToMap zeroes at `0x43ded4`. There is no autosave on this path.

Losing triggers on top of the reactor and the exit:
- Party death (state 8, `0x453bbd`): on `hive.blv` with bit 180 set and 237 clear, `EnterHouse(601)` runs instead of the death penalties.
- Change location (state 2, `0x454109`): when the map of the last save call is `hive.blv` with bit 180 set and 237 clear, `EnterHouse(601)` runs and the move is dropped.
- Save routine `0x44f320`: an autosave on `hive.blv` with bit 180 set and 237 clear is skipped (`0x44f39e`..`0x44f3ca`). Manual saves still go through. The routine records the map name before that check, which is why state 2 sees the hive.

## Planned design (not started)

- `src/Engine/MM6/MM6Finale.{h,cpp}`: reactor and queen death from an MM6-gated line in `Actor::Die`, boss removal on map load, the hive checks for party death, change location and autosave, and the ending close (oute3 at `MAP_START_POINT_PARTY` after a win, quit to the main menu after a loss).
- An MM6 ending window derived from `GUIWindow_GameOver`, with a protected constructor that skips MM7's certificate and a virtual `toggleAndTestFinished` for the 5 second wait and the popup. Global text by MM6 index, `win.str` and `lose.str` from the events archive.
- Hooks: `mm6EnterHouse` for 600/601 before the terminal refusal, `UIMSG_OnGameOverWindowClose` for MM6, the CHANGE_LOCATION branch in `Game.cpp`, `respawnDeadParty` in place of its throw, and `mm6AutoSave`.
- Tests: fixtures for bit 177 and item 544 with entry by `enterMapAtShippedEntrance("hive.blv")`, the reactor killed by a blaster (64), the queen killed, `hive:60` pressed with Space, the losing cases (no item, leaving through `hive:48`, party death), tapes for screen, map, bits 180/202/237, item 544, `MM6_Win.Pcx` and position.

## Open

House 602 has no caller in the exe or the data. The meaning of bit 4 in the flags dword at 6359000, which skips the first two movies, is unknown. Whether the MM6 blaster attack path damages the reactor in this engine is untested.

Privacy: addresses, IDs and map stems only.
