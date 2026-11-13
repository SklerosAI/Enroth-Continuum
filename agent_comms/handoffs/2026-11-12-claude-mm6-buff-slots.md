# Claude — MM6 buff slots, 2026-11-12 evening

- Owner: Claude Code, lane `mm6-buff-slots`, worktree `../enroth-worktrees/mm6-spells`, branch `claude/mm6-buff-slots`
  off master `d1634c355`.
- Recipient: the lead, for review and merge.
- Private trace: `local-game-data/private-wiki/mm6/BUFF_SLOTS_PE_TRACE_2026-11-12.md` (Git-ignored, every slot with
  its writers, readers and addresses).

## Commits

| Commit | What |
| --- | --- |
| `65d53d94a` fix(mm6): let a monster's Dispel Magic end every buff | Monster Dispel resets every shared party slot and, for each character who fails the save, every shared character slot. |
| `ff0fc275c` fix(mm6): halve an arrow for the shared Shield buff | The arrow halving reads `CHARACTER_BUFF_SHIELD`, not slot 3 (Earth resistance). |
| `138b1489a` test(mm6): keep party and character buffs across a fresh-process load | Gate pair for a buff save and a fresh-process load, no code change. |
| this file | Handoff. |

## The convention

Buffs live in the shared `PartyBuff` and `CharacterBuff` slots, which every MM6 writer, the HUD shelf and the stat
getters already used. No code in the port needs MM6's own slot numbers:

- MM6.exe's monster Dispel (0x40536f) clears every buff MM6 has, so the port resets every shared slot.
- The Shield check reads the shared Shield slot.
- The MM6 save is the port's own `.ec6` container. Its BUFF section stores all 20 party and 24 character records by
  shared index on both sides, so it already round-trips. Nothing reads or writes MM6.exe's save layout.

So no `MM6BuffSlots.h` mapping was added, it would have no caller. The MM6 to shared table is in the private trace
for a future importer of original saves.

## MM6.exe slot table (summary)

- Party array 16 entries at 9473588, 13 touched by expiry and Dispel, 11 used: 0 Fire, 1 Cold, 2 Electricity, 3 Magic,
  4 Poison, 5 Feather Fall, 6 Water Walk, 7 Fly, 8 Guardian Angel, 10 Wizard Eye, 11 Torch Light. 9 and 12..15 unused.
  Shared: 6, 17, 0, 13, 1, 5, 18, 7, none, 19, 16.
- Character array 16 entries at char+4712, 12 used: 0 Bless, 1 Heroism, 2 Haste, 3 Shield, 4 Stone Skin, 5 Luck,
  6 Intellect, 7 Personality, 8 Accuracy, 9 Speed, 10 Might, 11 Endurance. Shared: 1, 8, 7, 13, 14, 18, 17, 20, 15, 21,
  19, 16.

## Durations

MM6.exe Bless, Heroism and Shield: 3840 s + 300 L (Normal, Expert) or + 900 L (Master). The private
`rules/combat-buffs.json` says 3600 s (from the printed guide). The port casts Bless and Heroism through
`mm6::blessAndHeroismDuration`, the MM6.exe value. The json only feeds the combat-buff branch, which only Shield
could reach, and Shield is refused. The json was left alone.

## Tests

- New game tests: `MM6PlayerSpells.MonsterDispelMagicEndsEveryBuff` (Bless, Heroism at Expert, Protection from Fire,
  Fly and Protection from Magic cast from the spellbook, a Lich with its spell chance at 100 casts Dispel Magic),
  `MM6PlayerSpells.ShieldHalvesArrows` (Shield applied to the party by hand, since no verified cast exists, then an
  archer's arrow, with and without the buff), and the gate pair `MM6SaveAnywhereHeadless.DISABLED_BuffsSaveWithTheParty`
  and `DISABLED_BuffsComeBackInFreshProcess`, added to `tools/run_mm6_firstmap_gate.py`.
- Revert and fail: on the old Dispel the test fails with Protection from Magic still active and its shelf icon still
  drawn. On the old Shield check the shielded arrow takes 4 hit points instead of 2. The save pair has no fix to
  revert. Moving Fire resistance to another slot on load fails its buff and shelf checks.
- Unit tests 1,123 passed, 45 skipped. Full MM6 suite 202 passed, 2 skipped (the two `MM6ZSaveHeadless` research
  tests). The buff gate pair passes in fresh processes. The rest of the first-map gate wasn't rerun, the save runtime
  didn't change. MM7 controls not run, no shared-core file changed (`MM6MonsterAttack.h` is MM6-owned).
  `check_style` clean. `check_tidy` not run, no clang-tidy 22 locally.
- Capture `local-game-data/progress-captures/20261112-195711-mm6-buff-slots.png`: the first map with no buff active.
  Nothing off but the known missing food and gold digits next to the apple and the coins (HUD lane).

## For the lead

- Guardian Angel (MM6 party slot 8) has no shared slot. `MM6PartyDeath.cpp` reads `PartyBuff` 8, which is
  `PARTY_BUFF_HASTE`. Nothing writes that in MM6 today, Guardian Angel (50) and Haste (5) are refused. Every free shared
  party slot has a shared reader, so the slot is a choice for the lane that ports Guardian Angel. Not changed.
- `MM6MonsterAttack.h` reads actor buff 2 as MM6's Curse, which is `ACTOR_BUFF_SUMMONED` in the shared enum. Actor buffs
  were outside this lane. Not changed.
- Shield's case is traced (0x42472a) and could be enabled next.
- No original MM6 save and no `.ec6` exist in the owner's profile (only an old `autosave.mm7`), so no real save's buff
  block was read.
