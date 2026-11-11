# Claude — MM7 chain harness review fixes

- Owner: Claude Code lane subagent, while Codex is paused (owner approval 2026-11-10).
- Branch/worktree: `claude/mm7-chain-harness-fixes` stacked on `codex/mm7-chain-harness` (`a30db1654`), in Codex's paused `mm7-load-admission` checkout. The Codex branch ref is kept unchanged.
- Files owned: `test/Bin/GameTest/GameTestMain.cpp` (chain gate), `tools/run_mm7_campaign_chain.py`, `test/Bin/GameTest/GameTests_MM7CampaignChainC0.cpp`, `agent_comms/claims/codex-mm7-chain-harness.md` (policy text only).
- Purpose: fix review findings F1, F2, F4 and F5 in `reviews/2026-11-10-claude-mm7-chain-harness.md` and run the negatives that were not run.
- Started: 2026-11-10.
- Conflict check: Codex is paused. Claude's item lane owns only `GameTests_MM7LightItems.cpp`.
- Planned checks: C0 positive, C1-without-C0 and later-slot negatives, the legacy writer and reader pairs, runner resume, `check_style`.
- Handoff target: Codex root or Claude review, then integration.
