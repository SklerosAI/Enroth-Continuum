# MM6 interactive decorations and chest traps handoff

- Owner: Claude Code lanes, reviewed and integrated by the lead.
- `claude/mm6-decor-hover` `ac3aa9ef1`, on `master` as `810cae956`. `claude/mm6-chest-traps` `e5269e491`, on `master` as `404eacbd8`.

## Interactive decorations

`LevelDecoration::IsInteractive` and `GetGlobalEvent` only knew MM7's decoration rows. MM6 numbers its rows differently, so 316 trees on `oute3` counted as interactive and the ones past the 124th bound slot read `decorVars[-75]` on hover, a hard abort. MM6's own barrels and other interactive decorations did nothing. The MM6 executable uses rows 118-121, 146, 154, 155, 158, 162-164, 166, 167 and 182, binds variables at slot + 75 up to 124 like MM7, uses event base 400 instead of 380, and rolls a decoration's first event with `rand() % 100`. New `src/Engine/MM6/MM6Decorations.h`. A `decorationEventBase()` is used by hover, click, telekinesis and ChangeEvent, and for MM7 the ChangeEvent store writes the same byte as before. First-map RNG use now matches the original, which shifted later draws in four combat tests that pinned random outcomes (being re-pinned on `claude/mm6-combat-tests-rng`).

Tests: `MM6Decorations.HoverTreeShowsItsHint` and `BarrelNamesAndRunsItsGlobalEvent` (+1 Might from a real barrel, then the empty-barrel topic), plus unit tests, with revert-and-fail.

## Chest traps

Trapped MM6 chests never opened. Per the executable, with an active character the disarm score (skill, items, hirelings, mastery) is compared against `5 × MapStats difficulty` with `rand % 10`. Success clears the trap and opens the chest on the same click. Failure spawns one of four trap sprites between the chest and the party, and the next click opens the chest. When the sprite expires it damages characters within 1024 units for `5 + N d20`, through the MM6 damage channels, with a Perception avoid roll. 812 is Cold and 813 Electricity, the reverse of MM7's names. Flag changes reach the live chest and its sidecar together, so they survive a return and an EC6 v7 save. New `src/Engine/MM6/MM6ChestTrap.h`, an MM6 branch in `Chest.cpp` and a two-line expiry hook in `SpriteObject.cpp`.

Tests: `MM6ChestTrap.FailedDisarmSetsOffTrap`, `DisarmedTrapOpensChest`, `SprungTrapStaysGoneAfterReturn` on `outd3`, plus the save-anywhere gate children now disarm and check a chest across a fresh process.

## Checks on master

Decorations and traps 5/5, unit tests 1,117, MM6 first-map gate 29 with the MM7 controls, MM6 suite 104, MM7 in-process 107, full MM7 540/540, both sweeps clean, `check_style` clean. The four RNG-pinned combat tests fail until the re-pin lands.

## Open

Portrait expressions for trap avoidance. Trapped chests on maps other than `outd3` are not surveyed. The original's read of `decorVars` past 124 by file value is not ported, and no shipped map reaches it. Two MM6 indoor comments claim no 75 offset, which contradicts the executable.

Privacy: addresses, IDs and map stems only.
