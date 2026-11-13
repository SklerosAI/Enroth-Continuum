# MM6 campaign chain C6 handoff

- Owner: Claude Code lane C6, for the Claude lead. Branch `claude/mm6-chain-c6` in the `mm6-chain-c4` worktree,
  rebased onto `master` `eb18eec2d` (the Question prompts lane). Not pushed.

## Commits

1. `54d168338` The chain helpers that stand next to a target (decoration, floor item, corpse) now stand on the floor
   there instead of at the target's height. Delta sits on a ledge 128 units up, the party was still falling when the
   helper aimed, and pointing at the crystal failed. Test only.
2. `97e1d1047` Chapter C6 and its CMake line. Test only.
3. This handoff.

No engine change was needed. Revert check for commit 1: C6 on the old helper fails with "Failed to point mouse at
decoration #329" (Delta), seen on the first run of the chapter.

## C6 route (passing)

`GameTests_MM6CampaignChainC6.cpp`, from C5's `save005.ec6` on outc2 to `save006.ec6` on outc2.

- Castle Alamos: oute1 event 90 (Yes prompt) into cd1. Password plate event 69 (face 3273, sector 129) walked onto
  with `walkOntoMM6CampaignChainIndoorEventPlate`, Question answered `jbard` with `answerMM6CampaignChainQuestion`,
  status "Who told you!  Alright, you may pass!". The portal graph reaches Beta's sector 130 only through 129.
  Crystal Beta: event 59 on decoration 394, sets bits 54 and 192, item 551 in hand. Out by event 61 (Yes prompt) to
  oute1.
- Castle Darkmoor: outc3 event 91 into cd2. Crystal Delta: event 57 on decoration 329, a ledge in sector 170, sets
  bit 193, item 552. No key, bit or Question in its script. Out by event 60 to outc3.
- Castle Kriegspire: outb1 event 93 into cd3. Crystal Epsilon: event 62 on decoration 290, sets bits 104 and 194,
  item 553. Out by event 19 to outb1.
- Oracle through the High Council (outc2 event 49) as in C5. Modules 5, 15 and 17 take Beta, Delta and Epsilon in
  that order (bits 101-103 and 280, 281, 283 set, 192-194 cleared). The third sets the Oracle's topics to 76, 0.
  Topic 76 is not asked. Out by event 50 to outc2.
- Save slot 6.

Exit tapes: maps outc2, oute1, cd1, oute1, outc3, cd2, outc3, outb1, cd3, outb1, outc2, oracle, outc2. Bits (of 54,
100-104, 162-165, 167, 168, 192-194, 280-283) end at {54, 100-104, 162-165, 167, 168, 280-283}. Awards 2-7, 32, 58
unchanged. Items 551, 552, 553 each false, true, false. Oracle power (map byte 111) false, true, false, so C5's power
survives the save. Oracle topics 73, 74 then 76, 0. Total experience +1,200,000. Reputation 1600 unchanged. No
character at 0 HP.

## Fixtures

C1's party strength. Teleports to the three castle doors and the High Council door, inside cd1 to the side of the
password plate, next to each crystal and in front of each way out, inside the Oracle to the modules and the exit. No
Question fixture, the Question lane's helpers were on master when the chapter was written.

The boat to oute1 was not used. Only Silver Cove (outd1) has ships to oute1 (house 61 route 27 on weekday index 2,
house 63 route 31 on index 3). The save is 25 days in, index 4, and the outc2 ship to Silver Cove sails on 1 and 3.
Taking boats would mean days of resting first.

## Question records

- cd1 event 69 (password plate): answered by input, `jbard`, on the crystal's way.
- cd2 events 61-63 (sarcophagi, Yes/Y, a random item): not on Delta's way, not asked.
- cd3 events 13 (Guardian, 50,000 gold, the party has 46,703) and 27 (Curator): not on Epsilon's way, not asked.

## What the scripts say beyond the route (not played)

- cd1 teleporter event 47 to Silver Cove needs bit 54 and sits behind door 8, which needs key 569 from chest 0.
- cd2: forcefield faces 4522 and 4575 in the summoning room clear once sign 56 sets map byte 106. The portal graph
  reaches Delta without that room, a walk there was not tried.
- Private notes with all events: `local-game-data/private-wiki/mm6/CASTLE_CRYSTALS_SCRIPT_TRACE_2026-11-12.md`.
  The status rows for cd1-cd3 and crystals 10-12 in `QUEST_AND_AREA_STATUS_2026-11-12.md` are not updated, left
  for the lead.

## Checks

- MM6 chain resumed from C5's profile with `--through 6`: C6 passed with its pass marker.
- MM6 chain fresh, `--title mm6 --through 6`: C0 to C6 passed with pass markers, so commit 1 keeps C4 and C5
  passing. Profile kept for C7 at `/var/folders/qc/fg92hjk146s3g36x68b8n1lh0000gr/T/mm6-restart-chain-14996iuh`.
  The C5 profile `mm6-restart-chain-w2g2innk` now also holds a passing `save006.ec6`.
- Unit tests: 1122 passed, 45 skipped.
- MM6 suite `MM6*`: 188 passed, 2 skipped (the two `MM6ZSaveHeadless` tests that need private research files).
- `check_style`: clean. `check_tidy` not run, no clang-tidy 22 locally.
- MM7 controls: not run, no shared code changed.
- Capture: `local-game-data/progress-captures/20261112-195018-mm6-chain-c6.png`, the default first-map mode
  (New Sorpigal gate). View, minimap, compass, portraits, HP and SP bars, books and buttons read right. The food and
  gold boxes next to the apple and the coins are empty, the same first-map capture item the C5 and Question lanes
  reported. This branch changes no engine or HUD code.

No MM6.exe tracing was needed. Privacy: no game data, saves, captures or logs in Git.
