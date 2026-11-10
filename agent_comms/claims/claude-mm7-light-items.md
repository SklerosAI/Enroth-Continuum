# Claude — MM7 Light quest-item acquisition

- Owner: Claude Code lane subagent.
- Branch/worktree: `claude/mm7-light-items` from current `master`, in the `mm7-save-durability` checkout.
- Files owned: new `test/Bin/GameTest/GameTests_MM7LightItems.cpp` and its `test/Bin/GameTest/CMakeLists.txt` line. Runtime source only for a real defect, named first. Not `GameTestMain.cpp`, the chain C0 test or any existing `GameTests_MM7*.cpp`.
- Purpose: W4 fixture rank 3. Acquire through play the Light quest items every test currently adds directly: the Walls pillar keys first, then the Light and Dark altar pieces. The soul jar case and the Wine Cellar kill follow if the lane has room. Each helper becomes reusable by chain chapters C4 and C5.
- Started: 2026-11-10.
- Conflict check: Codex's chain harness lane owns `GameTestMain.cpp`, `GameTestOptions`, the runner and C0. No overlap apart from a one-line CMake addition.
- Planned checks: focused tests with an acquired-versus-not loop where meaningful, hand-in through the real advisor UI where cheap, full headless MM7 binary.
- Handoff target: Codex root for integration after Claude's review.
