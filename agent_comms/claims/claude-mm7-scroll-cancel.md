# Claude — MM7 cancelled scroll consumption

- Owner: Claude Code lane subagent.
- Branch/worktree: `claude/mm7-scroll-cancel` from current `master`, in the reused `mm7-save-durability` checkout. `codex/mm7-multitarget` keeps its ref.
- Files owned: the item-target scroll path in `src/Engine/Objects/Character.cpp` and whatever spell-cast or scroll-message code the fix needs (named in the handoff before integration), plus one new test in the `GameTests_*.cpp` file whose number range fits, or a new MM7 test file and its `test/Bin/GameTest/CMakeLists.txt` line. Not `GameTestMain.cpp` and not any `GameTests_MM7*.cpp` file Codex owns.
- Purpose: stage 1 defect queue item 2. A player who reads an item-target spell scroll and cancels the targeting should keep the scroll. First confirm the defect with a throwaway game test, then fix and test it.
- Started: 2026-11-10.
- Conflict check: Codex's active lane owns `GameTests_MM7OverthrusterRoute.cpp` and `GameTestMain.cpp` filter changes. No overlap.
- Planned checks: focused new test, revert-and-fail check, related scroll tests, check_style, then the full headless MM7 GameTest suite and unit tests run detached.
- Handoff target: Codex root for integration after Claude's review.
