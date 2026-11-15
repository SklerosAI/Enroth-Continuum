# Claude — MM6 first combat, 2026-11-12 evening

- Owner: Claude Code, lane `mm6-first-combat`, worktree `../enroth-worktrees/mm6-spells`, branch
  `claude/mm6-first-combat` off master `bef711cc3`. Master has moved on since (`307af7f5d`), the branch is not rebased.
- Recipient: the lead, for review and merge. Board row P6 of `agent_comms/MM6_PLAYTEST_2026-11-13.md`.
- Private trace: `local-game-data/private-wiki/mm6/FIRST_COMBAT_PE_TRACE_2026-11-12.md` (Git-ignored). Three static
  traces of MM6.exe with addresses for every claim: drinking potions (0x459040), rest and the passage of time
  (0x42c5c2, 0x42c67d, 0x4880a0, 0x47fee0), turn-based mode (0x405810, 0x4063D0, 0x435F12).

## Commits

| Commit | What |
| --- | --- |
| `dadf47ea2` fix(mm6): drink herbs and potions | `mm6::drinkPotion` (`src/Engine/MM6/MM6Potion.h`) applies MM6.exe's effects for items 160-188. Hook in `Character::useItem`. |
| `a0a7662d7` fix(mm6): play turn-based mode as MM6 does | MM6 turn indicator (`newhand1` / `newglas1` sprites at (444, 326)) in `TurnBasedOverlay::draw`, and no party movement phase in `NextTurn`, a round is 100 game seconds. |
| `1ff67ee07` fix(mm6): charge four food to rest on volcanic ground | `foodRequiredForTile` reads the MM6 tile's own tileset, volcano costs 4 (New Sorpigal has 528 such tiles). |
| `1e60f54a2` fix(mm6): play the error sound when the party lacks food to rest | Sound 27, no speech, as MM6.exe. |
| `f6ca18580` test(mm6): fight, rest and catch conditions on the first map | Guards for what already matched. |
| `3bb822a08` test(mm6): wait an hour, and capture the turn-based indicator | Wait 1 Hour guard, `MM6OffscreenFrame.DISABLED_TurnBasedCapture` on the offscreen allowlist. |
| this file | Handoff. |

All tests are in `test/Bin/GameTest/GameTests_MM6FirstCombat.cpp`, each starts a new party through the creation
screen's OK button on oute3.

## Items

| # | Item | State | Test |
| --- | --- | --- | --- |
| 1 | Melee | Works. A goblin hit with the A key and by clicking it dies, gives 56 experience, the body gives 8 gold and is removed. No miss line shows on the status bar, the original's miss text was not traced. | `MM6Melee.KillGoblinAndLootIt` |
| 2 | Turn-based mode | Fixed (indicator, movement phase, round length). Enter toggles, B passes, Enter leaves only while a character heads the queue (as MM6). | `MM6TurnBased.PartyAndMonstersTakeTurns` |
| 3 | Potions | Fixed, every MM6 herb and potion was refused. Cure Wounds +10 HP, Magic Potion +10 SP, Cure Poison clears the three poisons only, no empty bottle comes back, the empty bottle stays in the hand. | `MM6Potion.*` (3) |
| 4 | Resting | Fixed food on volcanic ground and the no-food sound. Rest and Heal on grass, the hostile refusal, the no-food refusal and Wait 1 Hour tested. Wait until Dawn and the rest encounter not tested, see the open items. | `MM6Rest.*` (5) |
| 5 | Conditions | Works. A spider poisons and a Large Rat diseases (fixture: special attack level 10), might drops. Temple cure is the services lane's. | `MM6Conditions.SpiderPoisonsAndRatDiseases` |
| 6 | HP/SP regeneration | Traced, not changed. MM6 has no natural regeneration, only items. Differences listed below. | none |

Monsters checked against MM6 `monsters.txt`: Goblin (76) and Common Rat (145) are Wimps and run when hurt, so the tests
spawn a stationary goblin and use the Large Rat (146, Normal). No throw was hit in any of these paths.

## Open, from the traces (not fixed)

- New party and the first night: the port starts a new party at one day without rest (`Party.cpp:118`, MM7's value),
  so a new party that waits past 3am without resting turns Weak and eats one food. In MM6.exe the only writers of the
  days-without-rest byte (9473388) are rest and heal (0x485113, sets 0), party death (0x453d8d, sets 0) and the day
  rollover (0x42c1b5, 0x42c1fe, 0x488256). Whether the MM6 new game zeroes it with the party struct was not traced,
  so this was left alone. The owner may hit it on the first night of the playtest.

- Turn-based start: MM6 rolls initiative 1/3/5 for monsters and 2..5 for every character, the port adds 16 and lets
  ready characters always go first. MM6 also stops the playing sounds before the enter/leave sound.
- Regeneration (0x4879f0): one point per call once 5 minutes passed, the port multiplies by the 5-minute marks crossed
  (a 1 hour wait gives 12 where MM6 gives 10). Pellinore (407) and Hades (415) do nothing in the port, it checks MM7
  item ids.
- Per-frame death rule: MM6 kills at HP <= -max HP every frame, the port at HP + base Endurance < 1.
- Rest details: the port clears recovery on Rest and Heal, stamps Sleep with tick 1 instead of now, and keeps a
  monster-slept character asleep after a finished wait. Rest proximity uses the port's hostility relation, MM6 the
  actor's hostility byte, not compared on data.
- No-rest days: MM6 zeroes HP and SP on the death roll and skips the insanity roll, decays reputation x0.99 a day.
- Rent Room is not wired for MM6 taverns (92..112), the shared handler asserts MM7's tavern range.
- Bonus potions (Energy, Protection, Resistance and their strong forms) set the bonus words that MM6 clears on rest
  through the temp-bonus reset. Not tested by input. The buff potions draw no buff overlay sprite.
- Potion mixing stays refused (`shouldBlockItemOnItemUse`), the matrix outcomes are not traced.

## Results

- `Run_UnitTest`: 1123 passed.
- Full MM6 suite (default `MM6*.*`, at `f6ca18580`): 212 passed, 2 skipped, 0 failed. The wait test and the capture
  came after and were run on their own.
- MM7 controls (`MM7*:VendorRefreshIntegration.*:Prs.Pr2354:Prs.Pr2956:Issues.Issue840`): 147 passed. Shared files
  touched: `Character.cpp`, `TurnEngine.cpp`, `TurnBasedOverlay.cpp`, `TileEnumFunctions.*`, `Outdoor.cpp`,
  `Game.cpp`, every change behind `GameTitle::MM6`.
- `MM6Rest.WaitAnHourNeitherEatsNorHeals` and the turn-based capture: passed on their own after the suite.
- Captures: `local-game-data/progress-captures/20261112-212003-mm6-first-combat.png` (standard first map) and
  `20261112-212005-mm6-first-combat-turn-based.png` (party's turn, hand at the lower right of the 3D view). The
  standard capture draws no food or gold numbers by the apple and the coins. The 20:39 visit-timers capture from
  another lane shows the same, and the turn-based capture, which renders later in the session, shows 7 and 200, so
  it looks like a first-frame artifact of the capture route, not a change on this branch. Not investigated. In the
  turn-based capture the stationary dummy goblin is drawn translucent gold, not checked against MM6.
- `check_style`: clean. `check_tidy` was not run, there is no clang-tidy 22 locally. Revert check: with the fix code reverted to `bef711cc3`, the two potion
effect tests, the turn-based test, the volcano test and the no-food test fail, the five guards pass.
