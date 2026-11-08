# MM7 monster second-spell projectile handoff

- Owner: Codex MM7 monster-spell lane
- Branch/worktree: `codex/mm7-audit-integration` in the existing `mm7-save-atomic` worktree
- Code commit after rebase: `e2eae3edf`; base: MM6 integration `2a773c462`
- Files changed: `src/Engine/Objects/Actor.cpp`, the focused MM7 test section in `test/Bin/GameTest/GameTests_1000.cpp`
- Result: MM7 monster spell projectiles now retain the `ActorAbility` passed to `AI_SpellAttack` through generic, Meteor Shower, Sparks and Sharpmetal creation. Impact code can therefore select the intended spell slot for damage and type. The MM6 generic launch retains its prior slot-one stamp; MM6 returns before the other three branches.
- Pre-fix proof: a separately preserved GameTest binary built before the source edit failed all three focused MM7 cases on the ability assertion. Generic and Meteor actual values were slot one; Sparks and Sharpmetal actual values were the sprite default attack one. The pre-fix binary's SHA-256 began `6ab9b6fe` and was never committed.
- Checks after the fix: network-denied throttled `OpenEnroth_GameTest` build passed; the MM7 monster-spell tests passed 3/3. Three MM6 combat children each passed 1/1 in fresh processes. Scoped cpplint on both files and `git diff --check` passed. The short GameTest window was handed to the combat lane.
- Remaining gate: the combined MM6 first-map chain and full headless MM7 suite are pending on the integration head after this commit and the separate MM7 combat audit branch land. No visual gameplay or installed helper check is claimed. Update the MM7 combat audit packet/index and append a separate wiki log after the combined gate confirms the source head.
- Privacy: tests use synthetic runtime state and local game data only. No private game files, saves, inventories or raw logs were copied into Git.
