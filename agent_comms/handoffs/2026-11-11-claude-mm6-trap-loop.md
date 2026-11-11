# MM6 indoor decoration parser (t7 trap loop) handoff

- Owner: Claude Code lane, reviewed by Claude lead. Integrated by the lead.
- Branch: `claude/mm6-trap-loop`, commit `30fd51a3e` on top of `claude/mm6-damage-channels` (already on `master` as `88619a690`). Cherry-picked onto `master`.
- Files: `src/Engine/Snapshots/MM6IndoorSnapshot.cpp` (MM6 only), `test/Bin/GameTest/GameTests_MM6Combat.cpp`.

## Cause

An MM6 BLV stores the decoration count, then every 28-byte record, then every 32-byte name. `projectMM6IndoorBase` read 60-byte records with the names interleaved, so most decoration slots were built from name bytes. In t7 that made 30 slots with every trigger flag set and 43 pointing at event 1, so `checkDecorationEvents` fired the trapped-door event 2,439 times per frame and killed the party. The original loader reads records then names (addresses in the commit message), and its per-frame decoration check is the same as ours, so the repeat itself was never wrong.

## Checks

- `MM6CombatHeadless.DISABLED_TrapDoorFiresOnlyWhenClicked`: enter t7, click the trapped door with the second character selected, and that character alone takes 50. Reverted, the first character falls to -121,750 in the first frame.
- Sweep on eight indoor maps: event 1 no longer runs on its own, real decoration events appear on d02, d05, d13, t2 and t6, and garbage missing-event references drop sharply. d02 now reaches decoration events on the event spell gate.
- Unit tests 1,093 passed, including the decoded MM6 indoor fixtures. MM6 suite 94 passed. All six `MM6CombatHeadless` tests passed alone. MM6 first-map gate passed with the MM7 controls. MM7 in-process 107 passed. `check_style` clean.

## Open

An EC6 v6 indoor save made before this fix stored decorations built from garbage. Its IDRT checks will probably refuse it now. There are no owner MM6 saves.

Privacy: addresses, IDs and map stems only. No game text or data in Git.
