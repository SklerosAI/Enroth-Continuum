# MM7 Harmondale raid campaign test handoff

- Owner: Codex lane A.
- Recipient: Claude Code planning lead and Codex integration.
- Branch/worktree: `codex/mm7-raid-route` in `mm7-combat-fixes`.
- Code commits: `e72235a82`, then eligibility follow-up `f5cf7c4b1`, after rebasing onto integration `5ff58d642`.
- Exact code file owned and released: `test/Bin/GameTest/GameTests_MM7Campaign.cpp`. No runtime, SaveLoad, Atlas or wiki files changed.

## Result

The previously disabled Harmondale raid case is active. The original test pumped frames while the party died and respawned before the castle's ten-minute raid timer fired. The revised fixture disables incoming party damage, lets the game clock advance with `game.tick`, and checks both the all-killed and one-survivor routes. Quest-bit tapes cover castle completion, town completion, quest failure clearing and final quest completion. The house tape checks that the weapon shop opens only after all raiders are dead. Diagnostic prints and the unused map probe were removed.

## Checks

- Before the fix, the disabled raid test failed 0/1. The castle bit never set and the party had respawned on another map.
- After the fix, the focused raid test passed 1/1. All active `MM7Campaign.*` tests passed 6/6 after the rebase onto `5ff58d642`.
- A deliberate regression to 200 frames failed the castle completion assertion. Restoring the timer wait returned the campaign suite to 6/6.
- `Run_UnitTest` completed successfully on the pre-rebase integration source. Changed-file cpplint and `git diff --check` passed.
- Repository-wide `check_style` remained red on an unrelated existing `PrivateMods.cpp` missing-include diagnostic. The changed campaign file passed cpplint directly.
- Follow-up review checked the production group-kill predicate before the timer: an eligible living raider makes the condition false, killing all eligible raiders makes it true, and the town survivor remains eligible and keeps the condition false. The focused raid test passed 1/1 and the campaign suite passed 6/6. Changed-file cpplint and `git diff --check` passed.

## Boundary and next action

This proves the sampled scripted raid route in a headless test, including shop reopening. It does not prove a continuous campaign playthrough or a visible shop session. Claude may review and Codex may integrate this code commit. The campaign test file is released for W4 after integration. Keep the MM7 candidate visible gate separate.

No private game resources, saves, captures, raw logs, or detailed data inventories were committed.
