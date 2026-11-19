# Claude MM6 GrayFace combat lane, 2026-11-18

Branch `claude/mm6-grayface-combat`, off `claude/mm6` at `eca9f3509`. Worktree `enroth-worktrees/mm6-indoor-light`.
Backlog items C18, C26 and C6 from `project/MM6_GRAYFACE_PATCH_REFERENCE.md`, whose rows now say what was done.

## Commits

- `8a152b0f3` Advance the MM6 turn-based clock 50 game seconds per round (C18). The MM6 branch of `NextTurn` advances
  213 ticks instead of MM6.exe's 426.67 (float at 0x4B9370, the value GrayFace rewrites). Flag
  `gameplay.fix_turn_based_turn_time`, default on. Monster movement per round is unchanged, it runs on the 64-tick AI
  timer. `MM6TurnBased.PartyAndMonstersTakeTurns` flips its pin to 213. New `MM6TurnBased.RoundTakesFiftyGameSeconds`
  runs three rounds with the flag on and off and checks that a walking goblin's path is identical in both.
- `1503dbc1c` Swing MM6 clubs and empty hands with the mace sound (C26). No flag, because MM6.exe already does it: its
  swing jump table (0x42A3AE, table 0x42A408) is indexed by the items.txt skill byte, where club and an empty or broken
  hand are 0 and play sound 83. The empty hand used to play the staff's 81 in our port. New
  `MM6Melee.ClubAndEmptyHandSwingLikeAMace`.
- `4df3efbc0` Make the MM6 recovery enchantment shorten recovery by 10% (C6). Flag `gameplay.fix_recovery_enchant`,
  default on. As GrayFace does, recovery no longer runs faster per tick in MM6, each new recovery is divided by 1.1,
  rounded, in `SetRecoveryTime` and in the turn-based queue (`_406457`). New
  `MM6Melee.RecoveryEnchantShortensRecoveryByTenPercent` and `MM6TurnBased.RecoveryEnchantShortensWaitForNextTurn`.

## Verified against MM6.exe and the data

GOG MM6.exe (sha1 `b078dea463b7a0b7e2c81a83955d446a3da2f6b2`, unpatched), disassembled with llvm-objdump:
- Items.txt skill parser at 0x4492A0..0x4494AA: staff 1, sword 2, dagger 3, axe 4, spear 5, bow 6, mace 7, blaster 8,
  shield 9, leather 10, chain 11, plate 12, club 0, anything else 13. Matches GrayFace's `_Skill_Club = -1` for MM6.
- Swing sound at 0x42A353..0x42A3F9, jump table 0x42A408: 0 and 7 play 83, 1 plays 81, 2 84, 3 85, 4 78, 5 80, 6 71,
  8 67. Bow shots force index 6 and blaster index 8.
- MM6 `dsounds.bin`: ids 67, 71, 78, 80, 81, 83, 84, 85 are the same swing and shoot sounds as MM7.
- Recovery enchantment: `Recover` 0x482BB0 (one caller, the Healing Touch case at 0x427EA9) and the per-frame loop at
  0x4885F9 both scale elapsed recovery by 1 + 50 x 0.01 for special enchantment 17 on any of the 16 equipped slots.
  GrayFace's readme says the vanilla enchantment didn't work. I found no reason for that in a static read, so the
  flag-off path keeps the old real-time 50%. Runtime MM6.exe behaviour was not observed.
- GrayFace hooks: `Hooks.pas` 2968..2974 and 2979, `IncreaseRecoveryRateProc` at 802, `RDiv` in `RSPak/RSQ.pas` 531.
- MM6 `items.txt`: clubs are 58, 59, 60. Item 533, MM7's Elven Chainmail id, is a Journal Fragment message scroll in
  MM6, so the inherited Elven Chainmail check can't fire.

## Fixtures

Club 58, Mace 50, a Helm 89 with special enchantment 17, all equipped in code. Monsters are a dummy goblin, or a
walking goblin 3000 units out for the turn-based path test. Everything else is input.

## Test results

- New and flipped tests: 5/5 pass with the fixes. With all three fixes reverted together and the flags left in place,
  all 5 fail (club swing tape `{83, 81}`, enchanted recovery 203 over 24 frames, turn-based initiative 99, round steps
  of 426).
- All MM6 game tests: 279 passed, 2 skipped (`MM6ZSaveHeadless`, private-research inputs), 281 run.
- Unit tests: 1125 passed, 43 skipped, 1168 run.
- MM7: 24/24 passed, the three controls `Prs.Pr2354`, `Prs.Pr2956`, `Issues.Issue840` plus every MM7 test that tapes
  recovery or turn-based mode (`Issue405, 663, 735d, 1175, 1301a, 1301b, 1315, 1341, 1447a, 1447b, 1447c, 1449, 1724,
  1989, 2002, 2075, 2116, 2289, 2551a`, `Prs.Pr2723`, `MM7Combat.DivineInterventionRecoveryFallsWithLightSkill`).
- `check_style` clean. `check_tidy` not run, clang-tidy 22 isn't installed here.
- The three commits were built and tested together. The intermediate commits were not built on their own.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/grayface-combat/before.png` and `after.png`: the rest screen after
twenty passed turn-based rounds from a new game's 9:00 am. The clock reads 9:33 am before and 9:16 am after.
