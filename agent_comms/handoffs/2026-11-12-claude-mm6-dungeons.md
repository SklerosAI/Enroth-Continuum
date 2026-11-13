# MM6 dungeons lane handoff, 2026-11-12

- Owner: Claude Code, MM6 dungeons lane (playtest board rows P7 and P5). Branch `claude/mm6-dungeons` in the
  `mm6-chain-c4` worktree, from `master` `f88268730`. Not pushed.

## Commits

1. `337a445c3` fix(mm6): let placed MM6 monsters hunt the party. `Actor::_SelectTarget` reads monsters.txt Hst for the
   party relation under MM6. Test `MM6Dungeons.TempleBatsBiteTheParty`.
2. `89174c8c5` fix(mm6): refuse rest near hostile monsters by MM6's own rule. `mm6::hostileMonstersNearParty`
   (`MM6PartyRest`), hooked in the rest message of `Game.cpp`. Test `MM6Dungeons.RestNearAndFarFromMonsters`.
3. `47c05f4ea` test(mm6): walk Goblinwatch, the Abandoned Temple and Gharik's Forge. `MM6Dungeons.AbandonedTemple`,
   `Goblinwatch`, `GharikForgeFloorAndChests`.
4. `78eb3b21b` test(mm6): aim at monsters that now move in the loot and reactor tests. `MM6Loot.DevilWarriorDropsPlans`
   and `MM6Finale.BlasterHurtsReactor` failed after commit 1 and pass again.
5. `d36b9a2c6` fix(mm6): light the alert gems for MM6's hostile monsters. `mm6::isMonsterHostileToParty`
   (`MM6PartyHostility.h`) in both alert blocks of `Actor.cpp` (also the minimap's hostile dots), shared with the rest
   check. Test `MM6Dungeons.TempleEntranceRaisesTheAlert`.
6. `5e39f4db6` test(mm6): clear a monster blocking a corpse in the chain helper. C4 stopped at the Temple of Baa
   priests once cultists crowd the party.
7. `107d1be50` test(mm6): put the spider queen's heart away only when it is in hand. `AbandonedTemple` passed alone
   and failed inside the full suite once.
8. This handoff.

## The big finding

Placed MM6 monsters never noticed the party. The party relation came from the hostility table, which
`HostilityTable::InitializeMm6` fills with FRIENDLY, and map load and spawn points start every monster at runtime
hostility FRIENDLY. In d02 the cobras stood next to the party without biting, and the party could rest anywhere. Only
test-spawned monsters (ACTOR_AGGRESSOR) ever fought. After commit 1 every non-peasant monster of a map comes for the
party, which changes play everywhere in MM6, outdoors too. The suite, the MM7 controls and the campaign chain ran after it
(results below).

## Verified against MM6.exe (notes in `local-game-data/private-wiki/mm6/DUNGEONS_PE_TRACE_2026-11-12.md`)

- Alert gems 0x401134: hostility byte non-zero, yellow under 5120, red under 307.2 (double at 0x4B9310).
- Rest refusal 0x42A680, called from 0x42C5D3: 2560 indoors, 5120 outdoors, octagonal distance 0x445420, aiState not
  4/5/11, hostility byte +0x3E non-zero, no line of sight.
- Party hunting reads the actor's hostility byte (AI 0x40203A and 0x4020AD, alert gem 0x401134). monsters.txt Hst is 4
  for 164 rows and 0 for 9. The shipped d02 record of the Queen Spider carries 4. Not traced: whether MM6's own map
  load keeps that byte (the load loop was not found). The fix reads monsters.txt instead, equal for any monster no
  script has touched.

## Per dungeon

| Dungeon | What a player does | State |
| --- | --- | --- |
| d02 Abandoned Temple | Enter by the oute3 door (Space) | Works |
| | Entrance bats bite, real fight (bat HP taped to 0) | Fixed (bats never attacked before commit 1) |
| | Rest near the bats: "There are hostile enemies near!" | Fixed (commit 2) |
| | Rest and heal 8 hours in the cage room, far from monsters | Works, HP back to full |
| | Doors 1 to 9 by click, each moves | Works |
| | Chests 2 (candelabra), 3, 5, 6, 1, all trapped, opened by click and an item taken | Works. Trap damage not asserted |
| | Angela's plate (event 14): SpeakNPC 195, she joins (hirelings 0, 1, 0) | Works |
| | Queen Spider (carries 481, its record's monster id field is 0) killed and looted, heart into a backpack | Works |
| | Teleporter 31 back to the entrance, exit 50 to oute3 | Works |
| | Cage fireball trap (event 22) | Open: the fireball launches and bursts but hurt nobody standing by the cage. Not checked against MM6 |
| | Sparks timer (event 30) | Not tested |
| d01 Goblinwatch | Janice's key, door 101 takes it (key goes to the cursor, not a backpack) | Works |
| | Gate hall goblins, real fight | Works (after commit 1) |
| | Code scroll in chest 1 (event 41, trapped, takes up to three clicks) | Works |
| | G on the combination panel opens vault doors 59 and 60 | Works. The other letters and the reset (P) not played |
| | Dais (event 64, Wizard Eye) click, no throw | Works |
| | Chests 2 to 10 opened, items taken | Works |
| | Exit 51, Janice pays 2000 | Works |
| d18 Gharik's Forge | Hot floor (event 51) sets map byte 105, burn timer hurts every character | Works (C2 never saw it fire) |
| | Cool floor (event 53) clears it | Works |
| | Bookcase (event 62): "Found something!" and an item | Works |
| | Chests 44 to 47, 49, 50 (letter 528, journal scrap 529) | Works |
| | Exit 90 | Works |
| | Levers 3 to 40, tiles 31 to 43, vents | Not played beyond the existing `D18Vent*` and C2 |

Fixture in the three dungeon walks: after entering, the party gets 2000 HP, 200 might and 200 accuracy. A new party
dies to a dungeon that now fights back. The first-room fights are real fights with that party.

## Side quests (P5)

| Quest | State |
| --- | --- |
| Candelabra (Potbello, bit 126) | Taken, found, handed in by input: +1000 gold, his thanks (npctext 307), bit cleared |
| Spider queen's heart (Buford, bit 130) | Same: +1000 gold, text 323 |
| Rescue Angela (Violet, bit 139) | Same: +500 gold, text 351, Angela leaves the party |
| Goblinwatch combination (Janice, bit 83) | Same: +2000 gold, bit 83 cleared, bit 300 kept |
| Chime of Harmony (bit 84, t1) | Not done |
| Rescue Sharry (bit 138, d03) | Not done beyond `MM6Houses.HouseNpcTopic` |

## Tests and gates

On `107d1be50`: unit 1123 passed, 45 skipped. Full MM6 suite 206 passed, 2 skipped. MM7 controls
(`MM7*:VendorRefreshIntegration.*:Prs.Pr2354:Prs.Pr2956:Issues.Issue840`) 147 passed, 2 skipped. `MM6Dungeons.*` 6
passed. Campaign chain C0 to C6 passed in fresh processes (C0 to C3 on `78eb3b21b`, C4 to C6 resumed from that profile
on `5e39f4db6`, the profile removed afterwards). `check_style` clean, without lua-language-server. `check_tidy` not run
(no clang-tidy 22 locally).

Revert and fail, each on its own build:
- Commit 1 reverted: `TempleBatsBiteTheParty` fails (no damage, no monster turns hostile), `RestNearAndFarFromMonsters`
  fails (nobody bites). `DevilWarriorDropsPlans` and `BlasterHurtsReactor` pass, which is how commit 4 was traced.
- Commit 2's `Game.cpp` hook reverted: `RestNearAndFarFromMonsters` fails, the rest screen opens and no status line.
- Commit 5's `Actor.cpp` hooks reverted: `TempleEntranceRaisesTheAlert` fails, the yellow alert stays off.

## Capture

`local-game-data/progress-captures/20261112-213745-mm6-dungeons-d02.png`, d02 at its entrance, taken before commit 5.
Food 7 and gold 200 drawn, compass, minimap, portraits and books in place. The four alert gems were green with the
entrance bats in range, which led to commit 5. The second capture `20261112-222111-mm6-dungeons-d02.png` (after
commit 5) still shows green gems: the map capture draws the frame right after `PrepareWorld`, before any game tick
computes the alert, while `TempleEntranceRaisesTheAlert` sees yellow after one tick. Nothing else in the frame is off.

## Open items

- Missing assets seen in the walks: `icons.lod/ib-bcu-a` during the d02 walk, `SPRITES.LOD/null` during the d18 walk.
- d02 cage fireball trap hurting nobody, d02 Sparks timer, d01 keypad letters other than G, d18 levers.
- Chime of Harmony and Rescue Sharry.
