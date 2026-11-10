# Claude — MM7 Xenofex real combat

- Owner: Claude Code lane subagent.
- Branch/worktree: `claude/mm7-xenofex-combat` from current `master`, in the `mm7-save-durability` checkout.
- Files owned: new `test/Bin/GameTest/GameTests_MM7XenofexCombat.cpp` and its `test/Bin/GameTest/CMakeLists.txt` line. Runtime source only if the fight exposes a defect, named in the handoff before integration. Not `GameTests_MM7LightXenofex.cpp` (Codex tape lane), not `GameTestMain.cpp` (chain harness).
- Purpose: W4 fixture rank 2. Replace the `Actor::Die(0)` Xenofex fixture with a real fight in Colony Zod that kills the actual Xenofex actor, then the map-leave completion. The helper becomes chain chapter C6's kill.
- Started: 2026-11-10.
- Conflict check: Codex's tape lane owns three Light test files. No overlap.
- Planned checks: focused test, a loop that leaves Xenofex alive versus dead, related Xenofex controls, full headless MM7 binary.
- Handoff target: Codex root for integration after Claude's review.
