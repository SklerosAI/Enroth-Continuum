# Claude — MM7 chain chapters C1-C3

- Owner: Claude Code lane subagent.
- Branch/worktree: `claude/mm7-chain-c1-c3` stacked on `claude/mm7-chain-harness-fixes` (`b915d9d4d`), in the `mm7-load-admission` checkout. Rebased onto `master` once the harness integrates.
- Files owned: new `test/Bin/GameTest/GameTests_MM7CampaignChainC1.cpp`, `...C2.cpp`, `...C3.cpp` (or one file for all three), their CMake lines, and the chapter table rows in `GameTestMain.cpp` only if the table needs them.
- Purpose: W4 continuous chain. C1 Harmondale arrival to castle goblins cleared and the repair request. C2 Stone City rescue to castle rebuilt. C3 messenger after the long rest, judge, Light chosen. Each loads the previous chapter's real save through the Load menu.
- Started: 2026-11-10.
- Conflict check: the integration lane works in the main checkout. The item lane owns `GameTests_MM7LightItems.cpp`. No overlap.
- Planned checks: the runner through C3 on a fresh profile, a rerun from a deleted C2 marker, the legacy pairs, full headless MM7 binary.
- Handoff target: Claude review, then integration.
