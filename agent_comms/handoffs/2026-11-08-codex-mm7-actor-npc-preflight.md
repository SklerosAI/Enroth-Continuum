# MM7 saved actor NPC ID preflight handoff

- Owner and recipient: Codex MM7 save reference audit lane to Codex integration lead.
- Branch/worktree: `codex/mm7-audit-integration` in the reused `mm7-save-atomic` worktree, originally based on main `9bccea648` and rebased onto integration `0c9b8f763` before merge.
- Code commit after rebase: `f39362f89`; owned files: `src/Engine/MM7/MM7SaveValidation.{cpp,h}` and `src/Engine/MM7/Tests/MM7SaveValidation_ut.cpp`.
- Result: detached indoor and outdoor actor validation accepts negative follower IDs for later roster-aware review, table NPC IDs 0..500, and generated NPC IDs 5000..5099. It rejects nonnegative 501..4999 and 5100 or higher before live reconstruction. Spell IDs and NPC table professions are untouched.
- Source boundary: `NPC.cpp` indexes the 501-row static NPC table below 5000 and the 100-row additional NPC table from 5000; `Actor_MM7::npcId` is signed 16-bit. Negative IDs use the dynamic follower roster, which this delta-only check cannot bound.
- Pre-fix proof: a preserved local pre-fix GameTest binary failed exactly the eight synthetic invalid-ID assertions for 501, 4999, 5100 and 32767 across both delta kinds. Valid controls -1, 0, 500, 5000 and 5099 did not fail. The binary was removed after the check.
- Checks: network-denied throttled GameTest build passed; focused MM7SaveValidation suite passed 17/17. Strict owner-only disposable MM7 writer and fresh-process reader passed 1/1 each under network denial; the temporary profile was removed. Scoped cpplint and `git diff --check` passed.
- Remaining: this is still a partial detached preflight, not the atomic writer's complete validator. Dynamic negative follower IDs, spell references and NPC table fields need separate evidence and tests. No production SaveLoad/UI wiring, full gate, or installed-app check is claimed.
- Privacy: synthetic test records only. No private saves, game data, inventories, captures or raw logs entered Git.
