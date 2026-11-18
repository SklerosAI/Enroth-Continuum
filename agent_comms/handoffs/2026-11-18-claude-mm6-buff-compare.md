# Claude MM6 buff-compare lane, 2026-11-18

Branch `claude/mm6-buff-compare`, off master `23f6eb0c9`. GrayFace backlog item C14 in
`project/MM6_GRAYFACE_PATCH_REFERENCE.md`.

## Commits

- `ffa75c155` Let a stronger MM6 buff replace a longer weaker one. `SpellBuff::Apply` takes a strictly stronger buff even when it ends sooner, for MM6
  behind `gameplay.fix_stronger_shorter_buff` (default on). The new buff replaces the old one whole, so the slot gets
  the new shorter expiry. Power alone is compared, mastery is not. Game test
  `MM6PlayerSpells.StrongerShorterBlessReplacesLongerOne`.

## Verified against MM6.exe, GrayFace and the data

- MM6.exe buff apply is 0x44A970. It refuses when the old expiry is nonzero and the new one is earlier (signed high
  dword, unsigned low dword compare), otherwise writes expiry, mastery (+10) and power (+8) from its arguments.
- GrayFace `FixCompareBuff` (`MMHooks.pas:2039`) hooks 0x44A97F after `or edx, eax`. With the old expiry nonzero it
  compares the new power word (`[esp+8+8+$10]`, the power argument) with the old power `[esi+8]` and jumps to the
  accept path 0x44A998 when the new one is greater (signed, strict). Equal power still falls through to the expiry
  compare. The MM7 entry (0x45852A, power `[ebp+$14]`, accept 0x45853F) and the MM8 entry (0x455DA8) are the same
  rule. MM7 is not changed here, the fix is gated on the MM6 title. No MM7 exe is on this machine, so the MM7 side is
  read from the GrayFace source only.
- Every MM6 buff write goes through `SpellBuff::Apply` (the only other write is the save loader). Live MM6 sources
  where powers can differ, so the fix can matter: player Bless and Heroism (level + 5), Feather Fall and Protection
  from Magic (rank x level), Torch Light (2, 3, 4 by mastery), the four protections (mastery x level), potions Bless,
  Heroism, Haste and Stone Skin (power 5, 6 hours), event Bless (power 0) and event Day of the Gods (stat buffs,
  (rank + 1) x level). Constant power, so the fix never changes them: Fly (1), Water Walk, Wizard Eye, Shield and the
  Wind and Water Master hirelings (0). No MM6 actor buff is reachable, the admitted player and monster spells apply
  none.
- The player-visible case: a Bless potion (power 5, 6 hours) then a Spirit 1 Bless (power 6, 69 minutes). Vanilla
  keeps Attack +5 on the stats screen, the fix shows +6.

## Fixtures

Spirit skill and the Bless spell on the cleric (Spirit 1 Normal) and the druid (Spirit 2 Normal), 100 mana each. The
Bless potion (item 177) is put into the knight's backpack. Drinking, casting, targeting and the stats screen are all
input.

## Test results

- Revert-and-fail: with the fix reverted, the stronger iteration fails three checks (power stays 5, no 69 minute
  Bless, Attack stays +5) and the weaker iteration passes. Restored.
- MM6 headless, `MM6PlayerSpells.*:MM6Potion.*:MM6Melee.*:MM6CharacterScreenHeadless.*:MM6Spellbook*.*:MM6Combat*.*`:
  42/42 passed.
- `OpenEnroth_UnitTest`: 1170 run, 1125 passed, 45 skipped (private-fixture cases without their environment inputs),
  0 failed.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840` plus every MM7 issue test that touches buffs (Issue405, 415a-e,
  417b, 442, 504, 506, 677, 784, 867, 1331, 1482, 1502, 1515, 1716, 1717, 2022, 2109, 2754, 2792, 2908 and
  `MM7EventSpell.DayOfTheGodsPowerMatchesOrdinaryCast`): 28/28 passed.
- `check_style`: clean. `check_tidy` not run, clang-tidy 22 is not installed locally.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/buff-compare/before.png` and `after.png`, Zoltan's stats tab after the
potion and the cleric's Bless. Before shows Attack +5 and Shoot +5, after shows +6 and +6. The temporary capture test
was removed.

## Found, not fixed

- Opening the character screen with I draws the stats tab for one frame before the inventory tab (the drawn text tape
  shows the stats rows on that frame). Not checked against MM6.
