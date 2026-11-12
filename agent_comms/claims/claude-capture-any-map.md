# Claude — progress captures from any map

- Owner: Claude Code. Owner's direct request on 2026-11-11: periodic headless captures from all over MM6 and MM7, not only the first map.
- Branch/worktree: `claude/capture-any-map` in `mm7-monster-combat` (the paused H lane's WIP stays committed on `claude/mm6-chain-harness`).
- Files owned: `test/Bin/GameTest/GameTests_MM7OffscreenFrame.cpp`, a new map capture test next to the MM6 sweep helpers in `test/Bin/GameTest/GameTests_MM6MapSweep.cpp`, the offscreen filter check in `test/Bin/GameTest/GameTestMain.cpp` (a few lines), and the local tools `~/enroth-tools/bin/enroth-capture` and `enroth-capture-loop` (outside Git).
- Test only. No `src/` changes.
- Handoff target: Codex root, lead, for review and integration.
