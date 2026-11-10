# Review — integrated MM7 W4 path GameTests

- Reviewer: Claude Code (lead), via a read-only review agent, findings spot-checked by Claude.
- Range: `1c5b47fbf..cbce587b0`, the `GameTests_MM7*.cpp` path files, the raid change in `GameTests_MM7Campaign.cpp`, and `GameTestMain.cpp`.
- Verdict: the harness is safe and every test file is free of debug output. The chapters each work from their own setup, but the seams between chapters are fixtured and never produced by a test. One seam may hide a save/load defect.

## Blocks the W4 claim

**F1. Chapter seams are written, never observed.**
- Adviser houses: `GameTests_MM7LightAdvisors.cpp` `seedAdvisors()` writes NPC 80-82 to `HOUSE_CELESTE_HOSTEL_1..3`. `MM7LightWalls.LightRoute` never tapes those houses.
- Robert's house: `GameTests_MM7LightAssassination.cpp` and `GameTests_MM7PitCubeAcquisition.cpp` write NPC 83 to `HOUSE_CELESTE_HOSTEL_4`. No adviser hand-in tapes it.
- NPC 80's first topic: `GameTests_MM7LightXenofex.cpp` writes it, then asserts the value it wrote. The Tolberti hand-ins check only NPC 80's house.
- The Light checkpoint handoff says the throne NPC's dialogue event ID "is reset on fresh Load" and dropped that assertion. Topics are serialized (`EntitySnapshots.cpp` `evt_A`/`evt_B` both ways), so a reset is either a map-load script rewriting it on purpose or a save/load defect. If it is a defect, a player who saves mid-chapter can lose the topic needed to progress. That is a stage 1 blocker until shown otherwise.

## Should fix

- **F2.** `interactWithLincolnEvent` in `GameTests_MM7OverthrusterRoute.cpp` swallows aiming failures and returns false for both "nothing clicked" and "click had no effect". Its two negative uses can pass without any interaction. Make it report that the pick landed on the face before the click.
- **F3.** `MM7LightWalls.ThroneClosedAtDawn` is negative-only. A drifted anchor passes it. Make it a two-iteration loop with the open case.
- **F4.** `House_Npc{}` index is `+ 1` in `GameTests_MM7LightAssassination.cpp` and `GameTests_MM7PitCubeAcquisition.cpp`. Buttons are 0-based (`UIHouses.cpp` button creation). Latent until Hostel 4 holds two NPCs.
- **F5.** The `MM7LightAdvisors` and `MM7LightXenofex` tests create no tapes, and Xenofex compares gold through a local saved before the action.
- **F6.** Comments that defend the change, a `//` block above a declaration, and semicolons in comments, across the Advisors, Xenofex, Overthruster, Walls, Scavenger and PitCube files.

Nits: unexplained tick counts in Walls, Campaign raid and Overthruster, an `EXPECT_GE` on a deterministic wetsuit count, an unchecked `pActors[0]` Xenofex identity, missing first-line comments in Advisors and Assassination.

## Harness

`GameTestMain.cpp` only acts with `--mm7-disposable-profile` or an exact listed filter, fails closed both ways, and RAM-fs runs never reach a native profile. No path to the owner's real MM7 profile was found. Writers require an empty profile, so chapters cannot chain today. The growing exact-filter list would read better as a table.

## Fixtures that matter most for "a player reaches the Light ending"

1. The F1 seams.
2. The Xenofex kill (`Actor::Die(0)`) and travel to Colony Zod.
3. Quest items never acquired in any test: pillar keys, both altar pieces, the soul jar case, the Wine Cellar kill bit, the scavenger items.
4. Goblin kills by `aiState = Dead` and blanket NoDamage. PitCube also writes party power directly.
5. Cross-map travel by teleport at Harmondale to Bracada, Celeste to Pit, Lambent to Avlee and Shoals to Lambent.

A continuous chained run would need a chain mode in the disposable-profile gate (load the previous chapter's slot, write the next), chapter tests that start from the Load menu with no state writes, closing tapes that assert the next chapter's entry state, and one documented party-strength fixture applied once instead of per-kill writes.
