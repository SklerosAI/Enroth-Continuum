# Claude — MM7 Light quest-item acquisition (paused)

- Owner: Claude Code lane, paused for the week on 2026-11-10. No agent is running.
- Branch/worktree: `claude/mm7-light-items`, WIP commit `88cbca0f5`, in the `mm7-save-durability` checkout. The worktree is clean.
- Files owned: `test/Bin/GameTest/GameTests_MM7LightItems.cpp` and its `test/Bin/GameTest/CMakeLists.txt` line. Not `GameTestMain.cpp`, the chain C0 test or any existing `GameTests_MM7*.cpp`.
- Purpose: W4 fixture rank 3. Acquire through play the Walls pillar keys, both altar pieces and the soul jar case, then the Wine Cellar kill.
- Started: 2026-11-10.
- Conflict check: Codex's chain harness lane owns `GameTestMain.cpp`, `GameTestOptions`, the runner and C0.
- Planned checks: build and run the three draft tests, behavior-guard checks, cpplint, `MM7LightWalls.*:MM7LightAdvisors.*:MM7LightSeam.*`, full headless MM7 binary.
- Handoff target: Codex root for integration after Claude's review. State in `handoffs/2026-11-10-claude-lead-week-wrap.md`.
