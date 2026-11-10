# Codex — MM7 Light Pit to Xenofex durable seam

- Branch/worktree: `codex/mm7-light-pit-xenofex-seam` from integration master `98ddaa6f4` in reused `mm7-death-regression` worktree.
- Owned code: new `test/Bin/GameTest/GameTests_MM7LightSeam.cpp`, its `test/Bin/GameTest/CMakeLists.txt` registration, and exact disabled writer/reader filter additions only in `test/Bin/GameTest/GameTestMain.cpp`.
- Goal: input-driven altar and Tolberti hand-ins produce NPC 80's natural throne/topic state; fresh-process Save/Load preserves it and can request Xenofex through UI.
- Boundary: no Walls, Claude scroll or shared runtime edits; no GUI or normal profile; private data/logs remain outside Git.
- Status: complete 2026-11-10; code `35f5063f5` and handoff `agent_comms/handoffs/2026-11-10-codex-mm7-light-pit-xenofex-seam.md`.
