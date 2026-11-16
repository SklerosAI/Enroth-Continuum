# Claude — MM6 rest and time rules, 2026-11-12 night

- Owner: Claude Code, lane `mm6-rest-rules`, worktree `../enroth-worktrees/mm6-spells`, branch `claude/mm6-rest-rules`
  off master `2b8813a4f`.
- Recipient: the lead, for review and merge, before the owner's first-night rest in tomorrow's playtest.
- Private trace: `local-game-data/private-wiki/mm6/REST_RULES_PE_TRACE_2026-11-12.md` (Git-ignored), with addresses for
  everything below. It builds on `FIRST_COMBAT_PE_TRACE_2026-11-12.md`.

## Commits

| Commit | What |
| --- | --- |
| `6e10fd7ba` test(mm6): weaken a new party on its first night without rest | Item 1. MM6 also starts at one day without rest, test only. |
| `ac5a871ba` fix(mm6): wake everyone when a wait ends | Item 3. MM6 branch in `Party::restOneFrame`. |
| `f45e035ef` fix(mm6): kill a character at minus his max HP | Item 4. MM6 branch of the per-frame check in `_494035_timed_effects`. |
| `6fa393da3` fix(mm6): run MM6's day change at 3 am | Item 5. `src/Engine/MM6/MM6NewDay.h`, hook in `_494035_timed_effects`. |
| `52d6950b7` fix(mm6): let monsters act before the party when turn-based starts | Item 6. MM6 branch in `stru262_TurnBased::Start`. |
| `d75b89eb5` fix(mm6): no rest encounter once a map has no monster left | Item 2. `mm6::restEncounterCanSpawn`, hook in `SpawnEncounterMonsters`. |
| `0a9939c39` test(mm6): fix two comments in the rest and death tests | Comment fixes. |
| this file | Handoff. |

All tests are in `test/Bin/GameTest/GameTests_MM6FirstCombat.cpp`, each starts a new party through the creation
screen's OK button on oute3, rests with the R key and the rest screen buttons, and lets time pass with `game.tick`.

## Items

| # | Item | State | Test |
| --- | --- | --- | --- |
| 1 | New party's days without rest | Traced, no change. MM6's new game reset (0x485f40) zeroes the byte and then runs the time update (0x4880a0), whose day change sees hour 0 to 9 and raises it to 1. So waiting past the first 3 am without rest makes the party Weak and eats a food in MM6 too. | `MM6Rest.FirstNightWithoutRestWeakens` |
| 2 | Wait until Dawn and the rest encounter | Wait until Dawn: 9:00 to 5:00, no food apart from the 3 am one, no encounter (waits never roll one). Encounter: fixed the MM6 gate, no encounter without a monster that can act and isn't a PeasantF1/F2/M1. The roll, the slot, one character woken, 60-65 minutes, no food, "Encounter!", sound 227 already matched. | `MM6Rest.FirstNightWithoutRestWeakens`, `MM6Rest.EncounterWakesOneSleeper`, `MM6Rest.NoEncounterOnClearedMap` |
| 3 | Wake, recovery, Sleep stamp | Wake fixed. Recovery and the Sleep stamp not changed, neither can show: a rest step is at least 6 game minutes and outlasts any recovery, and everyone wakes when the rest ends. | `MM6Rest.FinishedWaitWakesSleeper` |
| 4 | Death rule | Fixed: HP <= -max HP kills next frame, HP <= 0 knocks out, direct writes, SP kept. | `MM6Conditions.KnockedToMinusMaxHpDies` |
| 5 | 3 am rules | Fixed: MM6's day condition, reputation x0.99 truncated and clamped to +-1500, Weak without speech, death roll zeroes HP and SP and skips insanity. | `MM6Rest.NightWithAndWithoutRest`, `MM6Rest.DeathWithoutRestSkipsInsanity` |
| 6 | Turn-based start | Fixed the initiative: no +16, all characters ranked by attack recovery, so about a third of the monsters act first. The sound stop is not done, see below. | `MM6TurnBased.PartyAndMonstersTakeTurns` (extended) |

Fixtures: the encounter tests raise oute3's encounter chance from 10 to 100, the cleared-map test marks every monster
but the gated peasants dead, the death test gives every character endurance 100 and a Large Rat a fixed 100-damage
bite, the reputation test starts at 500.

## Open

- Turn-based Start and End call 0x48fb40(-1, -1) before sounds 207 and 206, inferred to stop every channel. Not ported,
  the headless tests have no audio device to show it. Start also zeroes the turn-based recovery override of all four
  characters by index, the port zeroes the slot of the queue position. Not ported, no test found.
- Encounter spawner differences left alone because the port's `SpawnEncounter` also places every map's monsters on
  load, so the change reaches all MM6 maps and needs the lead's call: MM6 rings monsters after the first at 128
  (outdoor) or 64 (indoor) around the spawn point where the port stacks them, MM6 caps the level odds at Dif 4 where the
  port caps at 5 (37 Dif 5 rows in MM6 mapstats, e.g. outa3, outb3, d20, hive), MM6 keeps the table hostility where the
  port spawns friendly aggressors. D20's slot 3 "0" spawns ArcherA through MM6's lookup fallback (inferred), the port
  shows "Encounter!" and spawns nothing.
- The five-minute regeneration tick still kills by HP + base Endurance < 1, which MM6 does only for the Hades drain.
  It differs only for an unconscious character who loses endurance.
- Map travel (`Game.cpp` around line 799) clears recovery through the shared `restAndHeal(Duration)`. MM6's travel
  (0x42c15b, 0x496180, 0x484d40) keeps it. Not touched, travel is another lane's.
- The speech MM6 doesn't play for conditions set by time (Weak, Dead, Insane, Unconscious) has no tape, so the new MM6
  paths are silent untested.
- The commit message of `d75b89eb5` gives the 1/3 constant as 0x4B9530, it is at 0x4B9430.

## Results

- `Run_UnitTest` binary: 1168 ran, 1123 passed, the rest skipped (private-data tests), 0 failed.
- Full MM6 suite (default `MM6*.*`, at `d75b89eb5`): 233 ran, 231 passed, 2 skipped (`MM6ZSaveHeadless.*`), 0 failed.
  After the comment commit the two touched tests were rerun and passed.
- MM7 controls (`MM7*:VendorRefreshIntegration.*:Prs.Pr2354:Prs.Pr2956:Issues.Issue840`): 149 ran, 147 passed,
  2 skipped, 0 failed. Shared files touched: `Party.cpp`, `Engine.cpp`, `TurnEngine.cpp`, `Indoor.cpp`, every change
  behind `GameTitle::MM6`.
- `check_style`: clean. `check_tidy` not run, there is no clang-tidy 22 locally.
- Revert check: with each fix reverted on its own, `FinishedWaitWakesSleeper` (Sleep stays on character 0),
  `KnockedToMinusMaxHpDies` (Unconscious instead of Dead at -78 of 78), `NightWithAndWithoutRest` (reputation stays
  500) and `DeathWithoutRestSkipsInsanity` (Insane on the death night), `PartyAndMonstersTakeTurns` (a character heads
  the queue, initiatives 17+) and `NoEncounterOnClearedMap` (an encounter spawns) fail. Restored, they pass.
- Capture: `local-game-data/progress-captures/20261112-222833-mm6-rest-rules.png`, standard first map by the New
  Sorpigal gate. Whole frame checked: 3D view, minimap, stained glass, books, food 7 and gold 200 by the apple and the
  coins, four portraits with full bars and green gems, "Rest" on the status bar from the cursor resting on the rest
  button. Nothing off. The capture route doesn't open the rest screen, so the fixes themselves aren't in the frame.
