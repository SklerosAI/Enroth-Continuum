# MM7 hired NPC profession preflight handoff

- Owner and recipient: Codex MM7 save reference audit lane to Codex integration lead.
- Branch/worktree: `codex/mm7-audit-integration` in the reused `mm7-save-atomic` worktree, rebased on main `1af5f586d` before editing.
- Code commit: `5fea72e32`; owned files: `src/Engine/MM7/MM7SaveValidation.{cpp,h}` and `src/Engine/MM7/Tests/MM7SaveValidation_ut.cpp`.
- Result: detached save preflight checks the 501 `NPCData_MM7` rows from `npcdata.bin`. A row flagged `NPC_HIRED` must have profession 0..77; its saved name pointer does not exempt it. Unhired rows are unchanged. The stale thumbnail description in the same header now matches the existing permissive implementation.
- Source boundary: `NPCData_MM7` is a 0x4C-byte record with raw flags and profession, `NPC_HIRED` is 0x80, and the shared profession table is indexed through 77. `setNPCNamesOnLoad()` restores static names after save reconstruction, while the MM7 follower roster uses the hired flag. Profession zero remains accepted pending dialogue evidence.
- Pre-fix proof: a temporary permissive detached-helper seam made the synthetic row 3 with `NPC_HIRED`, name pointer zero and profession 78 fail exactly its rejection assertion. Boundary profession 77 and zero, plus an unhired stale row 500, passed. The permissive helper was replaced before commit.
- Checks: network-denied throttled GameTest build passed; MM7SaveValidation suite passed 18/18. Strict owner-only disposable MM7 writer and fresh-process reader passed 1/1 each under network denial, and the profile was removed. Scoped cpplint and `git diff --check` passed.
- Remaining: this guards hired rows only. An unhired row linked from a visited house can also reach profession indexing; the stale row in the test is unreferenced only within that fixture. Profession zero and other NPC semantic fields need separate evidence. This partial detached preflight is not the atomic writer's complete validator, and production SaveLoad/UI wiring remains open.
- Privacy: synthetic NPC records only. No private game data, saves, inventories, captures or raw logs entered Git.
