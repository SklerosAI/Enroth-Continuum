# Codex — MM7 Pit control-cube acquisition

- Owner: Codex Pit cube lane.
- Branch/worktree: `codex/mm7-pit-cube-acquisition` in `mm7-load-admission`.
- Files owned: new `test/Bin/GameTest/GameTests_MM7PitCubeAcquisition.cpp`, its registration in `test/Bin/GameTest/CMakeLists.txt`, and this lane's claim/handoff.
- Purpose: use the real Light assassination request and Pit door, then kill and loot the cube-bearing actor through player input and hand in the acquired cube.
- Started: 2026-11-10 from integration `master` at `e1ac7b830`.
- Conflict check: existing Light assassination test remains untouched; the other active W4 lane owns Overthruster files.
- Planned checks: focused headless test and nearby controls, changed-file style, no graphical run.
- Status: complete on branch; ownership released after code commit `b31408129` and the dated handoff.
- Handoff target: Codex integration and Claude planning lead through a dated handoff.
