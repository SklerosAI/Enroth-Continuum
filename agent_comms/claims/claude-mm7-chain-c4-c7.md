# Claude — MM7 chain chapters C4-C7

- Owner: Claude Code lane subagent.
- Branch/worktree: `claude/mm7-chain-c4-c7` stacked on `claude/mm7-chain-c1-c3`, in the `mm7-death-regression` checkout (its `codex/mm7-light-pit-xenofex-seam` ref is integrated and kept).
- Files owned: new `test/Bin/GameTest/GameTests_MM7CampaignChainC4.cpp` to `...C7.cpp` and their CMake lines. The shared `MM7CampaignChain.{h,cpp}` helper only by additions, coordinated with the C1-C3 lane's follow-up.
- Purpose: W4 continuous chain to the Light ending. C4 Light chosen to the Walls hand-in. C5 the three adviser tasks and the Tolberti cube hand-in. C6 the Xenofex request, fight and hand-in, and the final task. C7 the Overthruster retrieval and the Light ending.
- Started: 2026-11-10.
- Conflict check: no overlap with the MM6 lanes or the recorder lane.
- Planned checks: the runner through C7 on a fresh profile, behavior guards per chapter, the legacy pairs, full headless MM7 binary.
- Handoff target: Claude review, then integration. Closing this lane closes the W4 row of the MM7 working gate.
