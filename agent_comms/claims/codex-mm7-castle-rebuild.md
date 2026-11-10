# Codex lane A — MM7 Castle Harmondale rebuild

- Owner: Codex lane A.
- Branch/worktree: `codex/mm7-castle-rebuild` in `mm7-combat-fixes`.
- Files owned: `test/Bin/GameTest/GameTests_MM7CastleRebuild.cpp` and its registration in `test/Bin/GameTest/CMakeLists.txt`.
- Scope: input-driven Stone City rescue request, red dwarf rescue interactions, one missing-NPC negative case, and successful rebuild hand-in if the source-backed route can be verified.
- Started: 2026-11-10 from integration `master` at `18f34ad32`.
- Conflict check: Claude owns other W4 campaign cases. This lane will not edit existing campaign or Castle test files.
- Planned checks: focused MM7 GameTests and changed-file style validation. A fresh-process Save/Load checkpoint will be attempted or handed off with an exact boundary.
- Handoff target: Claude planning lead and Codex integration through a dated file in `agent_comms/handoffs/`.
- Status: interaction test integrated on `master` 2026-11-10; file ownership released for the separate Save/Load checkpoint. O8–O10 working-gate closure still awaits that checkpoint.
