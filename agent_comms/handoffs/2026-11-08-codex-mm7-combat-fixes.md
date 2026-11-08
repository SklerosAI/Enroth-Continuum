# MM7 combat audit fixes handoff

- Owner: Codex MM7 combat lane
- Branch/worktree: `codex/mm7-combat-fixes` in the existing `mm7-combat-fixes` worktree
- Base after final rebase: integration `ed6122748`, a bridge-note-only descendant of the tested source `387b97a59`
- Commits after final rebase: `ba3392f7e` code/tests, `13a1e47f6` wiki page/index, `43b51a786` initial separate wiki log, `81c8e26f9` A2a audit status, `8bcadd0b7` integrated monster-spell documentation, `9bd772c8a` trace fixtures/test assertion, `b57e2347c` combined gate documentation, `e1f842502` separate combined gate wiki log
- Code scope: `Character.cpp`, `Monsters.cpp`, `CastSpellInfo.cpp`, new `GameTests_MM7Combat.cpp` and its CMake entry. The trace follow-up changes `GameTests_1500.cpp` and only `issue_1524.json` and `issue_1972.json`.

The five MM7 fixes cover armed and unarmed Break Weapon, Lich Mind/Body immunity, Divine Restoration's selected target, flaming archer `FireAr` parsing, and Light-skill reduction of Divine Intervention recovery. All five defect tests failed before the fix. Afterward the six-case MM7 combat suite passed 6/6, including the unarmed control. Three MM6 combat children passed 1/1 each in fresh network-denied processes. Title guards preserve the existing MM6 Break Weapon and Divine Intervention paths; MM6 direct potion use is blocked before the shared restoration case.

The separately integrated monster spell ability fix at `387b97a59` passed three focused MM7 GameTests and three MM6 controls. The combined branch then passed the full MM6 first-map gate: seven-child chain, retry fault children, paperdoll, normal Save/Load with production load/resave, and MM7 controls. The gate ran its children sequentially and overlapped other agents' GameTests while machine load remained tolerable.

The initial full MM7 headless suite found two trace changes caused by the integrated monster ability stamp. A binary built from monster-only `387b97a59` reproduced both; the pre-fix integration binary at `2a773c462` passed both. `Issues.Issue1524` needed new RNG states, and `Issues.Issue1972` needed new end HP. Retracing only those public JSON files twice produced identical hashes. Correct slot-two Meteor Shower damage changed party HP loss from 934 to 689, so the independent floor now requires at least 650 damage. The sound, map and 24-meteor assertions remain. Both isolated cases passed, then the full network-denied MM7 suite passed **467/469 with 2 skipped**.

After the docs-only final rebase, the six focused MM7 combat cases passed again 6/6 and the three MM6 combat children again passed 1/1 each in separate fresh processes, all under network denial. No runtime source changed after the full gate and suite.

Scoped cpplint on touched C++ files, JSON parsing and `git diff --check` passed. The broad style target previously failed on unrelated files. After final verification, main added only a bridge-note commit, so the tested runtime source is unchanged by the final rebase.

Remaining limits: the five combat fixes are not yet integrated. MM6's old unarmed Break Weapon path can still assert if later admitted; Claude owns that rule. Lich Spirit immunity is unresolved. The parser test checks the token, not live archer fire. Meteor Shower positioning remains open. No graphical acceptance or installed helper update is claimed. Private game data and local logs remain untracked.
