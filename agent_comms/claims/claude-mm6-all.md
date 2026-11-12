# Claude — all MM6 work

- Owner: Claude Code, assigned by the owner on 2026-11-11 to take all MM6 work and confirmed on 2026-11-12 as the sole MM6 delivery owner. Codex is the sole MM7 arbiter; neither title owner directs the other title.
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

Update 2026-11-12: under the owner's 5 + 5 worktree split, Claude runs these MM6 lanes:

| Lane | Branch / worktree | Files owned |
| --- | --- | --- |
| UI, MM6 screen audit | `claude/mm6-character-screen` in `../enroth-worktrees/mm6-charscreen` | `src/GUI/UI/MM6CharacterScreen.*`, MM6 branches of `UICharacter.cpp`, `UISpellbook.cpp`, `UIBooks.cpp`, `Books/*.cpp`, `UIQuickReference.cpp`, the MM6 icon mask in `ResourceManager.cpp`, new MM6 tests. Character screen merged at `2820d16e1`, books and spellbook next. |
| C2, chain chapter C2 | `claude/mm6-chain-c2` in `../enroth-worktrees/mm6-chain-c2` | new `GameTests_MM6CampaignChainC2.cpp`, additions to `MM6CampaignChain.{h,cpp}`, small MM6-owned fixes. |
| MSL, MM6 missiles | `claude/mm6-missiles` in `../enroth-worktrees/mm6-missiles` | MM6 missile sprite and Percival arrow handling, new MM6 tests. |

Update 2026-11-12 afternoon: merged today are the character screen, quick reference and rest, missiles, chain C2, spellbook, books and the winter ground fix. Active Claude MM6 lanes now:

| Lane | Branch / worktree | Files owned |
| --- | --- | --- |
| HUD, MM6 main HUD layout | `claude/mm6-hud` in `../enroth-worktrees/mm6-hud` | MM6 branches of `src/GUI/UI/UIGame.cpp`, the HUD buttons in `src/GUI/GUIWindow.cpp`, `src/GUI/UI/UIStatusBar.cpp`, the minimap rect in `src/Engine/Engine.cpp`, `src/GUI/MM6Hud*.h`, new MM6 tests. |
| HOUSES, MM6 house and shop screens | `claude/mm6-houses` in `../enroth-worktrees/mm6-houses` | `src/GUI/UI/Houses/MM6House.*`, `MM6Shop.*`, `src/GUI/UI/MM6HouseServiceData.*`, MM6 branches of shared house code and the house movie rect, new MM6 tests. |
| C3, chain chapter C3 | `claude/mm6-chain-c3` in `../enroth-worktrees/mm6-chain-c3` | new `GameTests_MM6CampaignChainC3.cpp`, `MM6CampaignChain.{h,cpp}`, the MM6 street NPC dialogue path, small MM6-owned fixes. |

Update 2026-11-12 evening: C4, the indoor lighting fix and the small leftovers are merged. The owner released shared MM6/MM7 code to Claude for today (Codex is idle), so a lane may change a shared-core file when the fix needs it and runs the MM7 controls afterwards. Five lanes are active, all five Claude worktrees in use:

| Lane | Branch / worktree | Files owned |
| --- | --- | --- |
| SPELLS, MM6 player spells batch 1 | `claude/mm6-spells` in `../enroth-worktrees/mm6-spells` | `src/Engine/MM6/MM6PlayerSpell.h`, `MM6SpellEffectGuard.h`, `MM6MonsterDamage.h`, MM6 branches of `CastSpellInfo.cpp`, `SpriteObject.cpp`, `Actor.cpp`, `UIGame.cpp`, new `GameTests_MM6PlayerSpells.cpp`. Shared core. |
| C5, chain chapter C5 | `claude/mm6-chain-c5` in `../enroth-worktrees/mm6-chain-c4` | new `GameTests_MM6CampaignChainC5.cpp`, additions to `MM6CampaignChain.{h,cpp}`, small MM6-owned fixes. |
| Q, MM6 Question prompts by input | `claude/mm6-questions` in `../enroth-worktrees/mm6-leftovers` | the MM6 Question dialogue code and its drawing, new MM6 tests, then the shared `Party.cpp` hireling name reset. Shared core. |
| EVSPELL, event spells 12, 35, 46, 83, 91 | `claude/mm6-event-spells` in `../enroth-worktrees/mm6-indoor-light` | the event branch of `MM6SpellEffectGuard.h` and the MM6 event spell effects, new MM6 tests on d01, d04, d08, d13, zddb08, znwc. |
| CC, Control Center access and blasters | `claude/mm6-control-center` in `../enroth-worktrees/mm6-control-center` | Oracle house 170 exit, sci-fi.blv blaster acquisition, the C7 and C8 route, new MM6 tests, small MM6-owned fixes. |

Conflict watch: SPELLS and EVSPELL both touch `MM6SpellEffectGuard.h` (player list and event list are separate functions). The lead merges SPELLS first and rebases EVSPELL.
