# Claude MM6 shared-coverage lane, 2026-11-18

Branch `claude/mm6-shared-coverage`, off `claude/mm6` at `eca9f3509`. Tests live in the new
`test/Bin/GameTest/GameTests_MM6GrayFaceShared.cpp`, suite `MM6GrayFaceShared`.

## Commits

- `0fbb1357b` test(mm6): cover GrayFace fixes that live in shared code. Seven input-driven MM6 tests:
  - C3 `ExplosionHitsParalyzedMonster`: a Percival arrow's explosion hits a paralyzed goblin, a stoned one stays at
    full HP.
  - C7 `ScholarAddsFivePercentExperience`: hire a Scholar through the street dialogue, kill a Goblin King, each member
    gets 52 instead of 50.
  - C21 `BlowSkipsDyingMonster`: a blow aimed at a dying goblin lands on the live goblin next to it.
  - C27 `HitWithoutRecoveryKeepsActiveCharacter`: with Endurance 350 the first member stays active through two thug
    hits, with normal Endurance he hands over.
  - W6 `WaitTicksRecovery`: after all four swing, Wait 1 Hour brings member 1 back while the Rest screen is open.
  - W17 `AwardsFollowActiveCharacter`: the awards page switches from "Returned the Prince" to "Ended Winter" on key 2.
  - W24 `AnimationFramesTurnWhenTheirTimeIsUp`: all 993 animated MM6 SFT sequences with loadable frames, and the TFT
    animations `john01` and `paladn01`, turn their frame at exactly the frame length.
- `b910f763c` fix(mm6): W4, `gameplay.fix_new_day_at_month_start`. `mm6::isNewDay` kept MM6.exe's `newDay > oldDay`
  compare, so Esc during "Wait until Dawn" on the last day of a month skipped New Day. The reference doc called this
  fixed, it wasn't for MM6. Test `NewDayAtMonthStartAfterWaitUntilDawnAndEsc` (mid-month and month-end iterations, the
  month-end one rests to the morning of day 28 and presses Esc right after Wait until Dawn).
- `8c23f3ae2` fix(mm6): W5, `gameplay.fix_sunrise_light`. `UpdateSunlightVectors` is MM6.exe 0x47BFE0 verbatim and
  stores the minute only by day, so 5:00 AM can keep an earlier hour's sun. Test `SunRisesAtFiveAm`. MM7 keeps the bug.
- `6377e05d4` fix(mm6): W18, `gameplay.fix_buff_duration_text`. MM6.exe 0x41A2F0 and our `MakeDateTimeString` drop
  minutes when days are present. Buff times now show the two largest nonzero units. Test
  `BuffDurationShowsMinutesNextToDays` pins "1 Day 4 Minutes" (25 hours less 55 minutes waited and a few seconds).
- This commit: reference doc rows C3, C7, C21, C27, W4, W5, W6, W17, W18, W24 and the two action rows, plus this
  handoff.

## Verified against MM6.exe and the data

- MM6.exe disassembly (objdump on the GOG exe): 0x47BFE0 sunlight update and its callers 0x41D546, 0x469F97,
  0x46A1AD (the minute compare against outdoor +0x7C264), 0x41A2F0 buff duration text with the skips at 0x41A472 and
  0x41A4DE that GrayFace NOPs. GrayFace source `MMHooks.pas` lines 3120, 3632, 3726, 4091, and the readme entries for
  W4, W5 and W18.
- MM6 data: `monsters.txt` (Goblin 76 is a Wimp and runs, Thug 64 is Normal with no special, Goblin King 78 gives 200
  experience), `npcprof.txt` (Scholar is profession 4, 500 gold), `awards.txt` (1 and 7), `dsft.bin` (6455 frames,
  `Lntnrev0` names `Lntrev1` to `Lntrev5` which sprites.lod lacks, skipped), `dtft.bin` (19 frames, three animations,
  `mossrk_2` is a test sequence with repeated names and is not checked). Only MM6 was in scope.
- New Sorpigal's 38 street NPCs at the test seed have no Scholar, so the test turns Shelley into one before hiring her.

## Revert-and-fail

Each shared fix was reverted in one batch and every coverage test failed: C3 (`CanBeDamaged` to `CanAct` in
`evaluateAoeDamage`), C7 (Scholar line in `learningPercent`), C21 (target check in the melee attack), W6 (no
`Recover` while a rest type is set), W17 (awards refill), W24 (`>=` to `>` in both frame tables). C27 passed with only
the `SetRecoveryTime` early return removed, because `switchToNextActiveCharacter` also keeps a ready member. With both
guards removed the tough iteration switches on the second hit and fails. W4, W5 and W18 fail on base code and pass
with the fixes.

## Fixtures

Percival bow and Bow skill on member 1, two bystander goblins at 100 HP (a goblin would die to the explosion and dying
ends a paralysis), paralysis and stone applied directly. A 1 HP Goblin King and gold for the Scholar. Health 200 and
Endurance 350 for C27. Award bits set directly. Food 40 and encounter chance 0 for the four-week rest in W4. Fire 25
Expert on the sorcerer for W18. Everything else is input.

## Test results

- `MM6GrayFaceShared.*`: 10/10.
- All MM6 game tests (`MM6*`, one process): 285 passed, 2 skipped (private-fixture env), 0 failed, 126 disabled.
- `OpenEnroth_UnitTest` with the MM6 data paths set: 1168 passed.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840:Issues.Issue415*:Issues.Issue1068`: 9/9.
- `check_style` clean. `check_tidy` not run, clang-tidy 22 isn't installed locally.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/shared-coverage/before.png` and `after.png`: New Sorpigal's gate on the
first frame of 5:00 AM after waiting from 9:00 to 4:55. Before is lit like 9 AM, after is dawn.

## Not fixed

- MM7 has the same 5:00 AM sunlight code. Codex's call.
- Tab doesn't switch members on the MM6 character screen, the digit keys do.
