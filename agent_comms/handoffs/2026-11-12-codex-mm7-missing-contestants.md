# MM7 Missing Contestants handoff

- Owner: Codex, sole MM7 delivery owner.
- Branch/worktree: `codex/mm7-dark-chain` in `mm7-combat-fixes`.
- Code: `296704777`; wiki: `296799abd`; log: `8db77da90`.
- Integration: neutral `master` merge `7058a30b5`.

## Scope and evidence

`MM7SideQuests.MissingContestantsRequiresDragonCaveShield` follows Markham's
real offer, proves the no-shield refusal, clicks the physical Contestant Shield
on the floor of Emerald Island's Dragon's Lair, and completes the return. It
asserts item consumption, quest clearance, 1,000 gold, 1,000 XP per character
and the award for all four characters.

The exact disposable-profile writer used production MM7 Save and structural
preflight. A separate process selected `save000.mm7` through the normal Load
menu and verified the final state; the slot digest stayed unchanged. Missing
profile and empty-reader-profile invocations were rejected. The disposable
profiles were moved to Trash after verification. The reconciled regional
side/promotion selection passed 11/11 headlessly and `check_style` passed
157/157.

## Boundary

This is one independent regional quest boundary, not continuous regional
progression. Private game data, the save, checksum and raw logs stayed outside
Git. The owner's headless-only instruction remained in force.
