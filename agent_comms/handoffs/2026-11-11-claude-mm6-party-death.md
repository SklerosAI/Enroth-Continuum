# MM6 party death handoff (M3 part 1)

- Owner: Claude Code lane, reviewed and integrated by the lead.
- Branch: `claude/mm6-party-death`, rebased onto `master` `7f279f9dc`, commits `6f24df656` and `bde0e640c`, cherry-picked onto `master`.
- Shared core: one MM6-gated hook in `Game.cpp` that calls `mm6::respawnDeadParty()` before the MM7 handler. MM6 logic is in `src/Engine/MM6/` (`applyPartyDeath`, `respawnDeadParty`).

## Original rules (MM6 executable)

A party dies when nobody can act and the rest screen is closed. The handler clears hirelings and NPC-table hired flags by the join column, plays the losing movie, raises the death count and gives award 82 to each character. It then advances 7 days, zeroes days without rest and clears only the water-damage flag. Gold drops to 0, or is halved while Guardian Angel is active. Buffs and conditions are cleared and turn-based mode ends. HP is set to 1, or half or full maximum with Guardian Angel at Expert or Master. The first character is made active. The party is placed at the New Sorpigal fountain on `oute3`, loading that map if needed, and monsters are reset with their table hostility instead of MM7's all friendly. A random character speaks and the status bar shows the cheated-death line. On the hive map with the reactor quest state, the original plays the losing ending instead. Addresses are in the commit messages.

## Checks

`MM6PartyDeath.DiesInDungeonWakesInNewSorpigal` and `DiesOnFirstMapWakesWithoutReload`, from New Game by input, are in the normal MM6 suite, and each passes alone. They check the maps, deaths, gold, days, status bar, position, HP, award, the killers' hostility and that the party can move afterwards. Revert-and-fail. Unit tests cover plain death and Guardian Angel at each mastery. MM6 combat tests 17/17, MM6 suite 102, first-map gate 27 with the MM7 controls, MM7 in-process 107, full MM7 540/540, unit tests 1,110, `check_style` clean.

## Open

The hive losing ending throws an explicit error (M4 finale). Award 82 text has an unrendered placeholder. Guardian Angel has no MM6 cast yet, so it is unit-tested only. NPC-table dismissal has no game test. In-game Load, quick save and autosave are the rest of M3.

Privacy: addresses, IDs and map stems only.
