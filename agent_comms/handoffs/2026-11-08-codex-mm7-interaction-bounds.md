# MM7 actor group dialogue bounds handoff

- Owner: Codex MM7 save reference lane
- Branch/worktree: `codex/mm7-audit-integration` in the existing `mm7-save-atomic` worktree
- Code commit after rebase: `88a0334c4`; claim commit after rebase: `4651fcdc0`
- Files changed: `src/Engine/Interaction.cpp`, `test/Bin/GameTest/GameTests_1000.cpp`
- Result: `InteractWithActor` checks an actor's group against the 51-entry NPC group table and its nonzero phrase index against the 300-entry catchphrase table before either lookup. The nonzero `npcId` dialogue path and valid group/phrase behavior are unchanged. Saved actor groups retain their other spawn and event meanings.
- Pre-fix proof: two separate MM7 first-map GameTest processes each exited on the expected libc++ bounds assertion, one at group 51 and one at phrase 300.
- Checks after the fix: network-denied `enroth-build OpenEnroth_GameTest` passed; the two new MM7 tests and the existing actor-ten control passed 3/3; the MM6 startup/chest control passed 1/1. Scoped cpplint and `git diff --check` passed. GameTests ran serially and the lane was handed to the MM7 combat agent afterward.
- Rebase: rebased over integration `babe49b27` after those checks. That integration advance changed MM6 shop and house files, not either owned file; no post-rebase GameTest was run while the combat lane used the serial test window.
- Remaining: this closes only the two live group-news table reads. Positive and negative actor `npcId` references, other NPC semantic fields, a complete native-save validator, trusted profile root and production atomic SaveLoad wiring remain open. The MM6 control was a startup isolation check, not a direct MM6 actor-group interaction test.
- Privacy: only synthetic bounds and high-level local test results are recorded; no private saves, game bytes, text or inventories were copied into Git.
