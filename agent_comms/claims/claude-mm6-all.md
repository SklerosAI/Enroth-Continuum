# Claude — all MM6 work

- Owner: Claude Code, assigned by the owner on 2026-11-11: "you're taking all of the MM6 work". Codex keeps the coordination lead and MM7.
- Claude plans the MM6 lanes, reviews them and integrates them into `master` one at a time, with the M7 controls from `PLAN.md` after every merge that touches a shared-core file.
- Started: 2026-11-11.

| Lane | Branch / worktree | Files owned |
| --- | --- | --- |
| F, hive finale | `claude/mm6-finale` in `mm7-save-durability` | MM6 branches of `src/Application/GameOver.cpp` and `src/GUI/UI/UIGameOver.cpp`, MM6 finale code under `src/Engine/MM6/`, the hive branch of MM6 party death, new MM6 tests. Shared core. |
| H, MM6 chain harness and C0 | `claude/mm6-chain-harness` in `mm7-monster-combat` | MM6 gate rows in `test/Bin/GameTest/GameTestMain.cpp`, `GameTestOptions.{h,cpp}`, new `MM6CampaignChain.{h,cpp}` and `GameTests_MM6CampaignChainC0.cpp`, the title switch in `tools/run_mm7_campaign_chain.py`. Shared harness. |
| X, house exits and NPC 12 rooms | `claude/mm6-house-exits` in `mm7-load-admission` | `src/GUI/UI/Houses/MM6House.{h,cpp}`, `src/GUI/UI/MM6HouseServiceData.{h,cpp}`, new `src/Engine/MM6/MM6HouseExit.h`, new MM6 tests. |
| SKY, outdoor sky placeholder | `claude/mm6-sky` in `mm7-death-regression` | MM6 sky texture selection (`src/Engine/Graphics/OutdoorSkyResources.*`, the MM6 path in `src/Engine/Graphics/Outdoor.cpp`, MM6 code under `src/Engine/MM6/`), a new MM6 test. Shared core. |

Conflict check: no file overlaps between the four lanes. None of them touches Codex's MM7 Dark-chain test files.

Update 2026-11-11 afternoon: H is integrated (`133fabf5a`, full MM7 suite 540/540) and SKY is integrated (`98cb96ab3`, MM6 suite 116, full MM7 suite 540/540). Their worktrees host two new lanes:

| Lane | Branch / worktree | Files owned |
| --- | --- | --- |
| C1-C4, MM6 chain chapters | `claude/mm6-chain-c1-c4` in `mm7-monster-combat` | new `GameTests_MM6CampaignChainC1.cpp` to `C4`, additions to `MM6CampaignChain.{h,cpp}`, CMake lines, small MM6-owned engine fixes in separate commits. |
| VISIT, MM6 map visit record | `claude/mm6-visit-record` in `mm7-death-regression` | the MM6 visit block read and write (new files under `src/Engine/MM6/`, the MM6 EC6 sections carrying map deltas, thin hooks in `Outdoor.cpp`, `Indoor.cpp`, `SaveLoad.cpp` and the MM6 map transition module), new MM6 tests. Shared core. |

Queued: bright white ground near the `oute3` start and in patches on `outd3` (seen before and after the sky fix).

Update 2026-11-11 end of day: all lanes finished and are on master (F, H, X, SKY, C1, BOW, VISIT). No MM6 lane is active. Week wrap and the queue for next week: `handoffs/2026-11-11-claude-mm6-week-wrap.md`. The claim on MM6 as a whole stays with Claude.
