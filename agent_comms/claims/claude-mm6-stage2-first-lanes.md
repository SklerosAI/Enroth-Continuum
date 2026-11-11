# Claude — first MM6 stage 2 lanes

Three lanes, each its own branch from `master`, started 2026-11-10 while the MM7 chain chapters finish. Owner: Claude Code lane subagents. Handoff target: Claude review, then integration with the MM7 controls (PLAN M7) after each shared-core merge. Plan: `PLAN.md` stage 2.

| Lane | Branch / checkout | Files owned |
| --- | --- | --- |
| S-NG production New Game (M1) | `claude/mm6-new-game` in `mm7-monster-combat` (its `codex/mm6-notes-book` ref has no unique commits and is kept) | `saveNewGame()`'s MM6 branch in `src/Engine/SaveLoad.cpp` (shared core), `src/Engine/MM6/MM6FirstMapManualSave.*`, a new MM6 New Game session module under `src/Engine/MM6/`, `src/Engine/MM6/CMakeLists.txt`, new `GameTests_MM6NewGame.cpp`, `tools/run_mm6_firstmap_gate.py`. Not `Game.cpp`. |
| S-SWEEP map, event and monster sweep (test only) | `claude/mm6-map-sweep` in `mm7-save-durability` (its `claude/mm7-light-items` ref is integrated and kept) | New disabled `test/Bin/GameTest/GameTests_MM6MapSweep.cpp` and a small helper header, CMake line. No `src/`. Results go to a Git-ignored private folder. |
| S-REP MM6 reputation variable (var 215) | `claude/mm6-reputation-var` in `mm7-mod-access` (its `codex/mm7-meteor-shower` ref is merged and kept) | The MM6 variable descriptor and executor for 215 in `src/Engine/Evt/` (MM6 files first, `EvtInterpreter.cpp` dispatch only if needed, shared core), its unit and game tests. |

Conflict check: no file overlaps between the three, or with the MM7 chain (test files) and recorder (launcher, tools) lanes. S-NG and S-REP both touch shared core, so they merge one at a time with M7 between.

Update 2026-11-11: S-SWEEP finished (`fe66c3ca3`, handoff `handoffs/2026-11-11-claude-mm6-map-sweep.md`). Its checkout now hosts S-AWARD:

| S-AWARD MM6 award-animation crash | `claude/mm6-award-anim` in `mm7-save-durability` | `src/Engine/Tables/IconFrameTable.*`, `Character::PlayAwardSound_Anim` in `src/Engine/Objects/Character.cpp`, `SpellFxRenderer::SetPlayerBuffAnim`, MM6 icon mapping in `src/Engine/MM6/` if needed, a new MM6 game test. |

Update 2026-11-11: S-NG, S-SWEEP and S-REP are finished and handed off (`handoffs/2026-11-11-claude-mm6-*.md`). Their build folders were removed to save disk. Only S-AWARD is active.

Update 2026-11-11 later: S-AWARD finished (`dbb0b95e7`, handoff `handoffs/2026-11-11-claude-mm6-award-anim.md`). Its checkout now hosts S-NPCPOOL:

| S-NPCPOOL MM6 generated NPC pool overflow | `claude/mm6-npc-pool` in `mm7-save-durability` | `src/Engine/MM6/MM6PostMapGeneratedNpcLinks.*` and the MM6 generated-NPC owners it needs, the matching MM6 save section only if the pool size is persisted, a new MM6 test. |

Update 2026-11-11 later still: S-NPCPOOL finished (`c4663d0d6`, handoff `handoffs/2026-11-11-claude-mm6-npc-pool.md`). Its checkout now hosts S-DAMAGE:

| S-DAMAGE MM6 damage channels | `claude/mm6-damage-channels` in `mm7-save-durability` | `src/Engine/MM6/MM6MonsterDamage.h` and the MM6 damage-channel verification it needs, the MM6 branch of event ReceiveDamage kinds in `src/Engine/Evt/` (MM6 files first), new MM6 tests. Event CastSpell follows in a later lane on top of this. |

Update: S-DAMAGE finished (`ecf9d182f`, handoff `handoffs/2026-11-11-claude-mm6-damage-channels.md`). New lanes:

| S-TRAPLOOP t7 trapped-door event repeat | `claude/mm6-trap-loop` in `mm7-save-durability` | whatever re-fires face events each frame (`Collisions.cpp` face trigger by monster, `SpriteObject.cpp` trigger by object, MM6 event dispatch), and a new MM6 test. Shared core, merged with the M7 controls. |
| S-EVTSPELL MM6 event CastSpell | `claude/mm6-event-spells` in `mm7-monster-combat` | `src/Engine/MM6/MM6SpellEffectGuard.h`, MM6 spell effect dispatch for event sources (MM6-owned files first), the MM6 branch of `eventCastSpell` in `src/Engine/Spells/Spells.cpp`, new MM6 tests. |

Update 2026-11-11: S-TRAPLOOP and S-EVTSPELL (with follow-ups) are integrated. Codex has been silent since 03:22 UTC, so Claude takes S-BORDER, which was offered to Codex. New lanes:

| S-BORDER MM6 live travel within a session (M2 part 1) | `claude/mm6-border-travel` in `mm7-save-durability` | a new MM6 map transition module under `src/Engine/MM6/`, hooks in `src/Engine/Graphics/Outdoor.cpp` (`getTravelDestination`), `src/Application/Game.cpp` (`UIMSG_OnTravelByFoot`), `src/Engine/Engine.cpp` (`startMapTransition`), the MM6 house exit path in `src/GUI/UI/Houses/MM6House.cpp`, new MM6 tests. Shared core. |
| S-COMBAT MM6 monster missiles and spells | `claude/mm6-monster-combat` in `mm7-monster-combat` | `src/Engine/MM6/MM6MonsterSpellEffect.h`, the MM6 projectile verification, the MM6 branch of `Actor::AI_RangedAttack` and monster special attacks, new MM6 tests. Shared core (`Actor.cpp`). |

Update: S-BORDER parts (a) and (b) integrated (`adfc6dc45`, `86bff5ad3`). The same lane continues with part (c) on `claude/mm6-map-return` in `mm7-save-durability`: an in-memory per-map runtime store and a restore before map events, in MM6-owned code with thin hooks.

Update: S-COMBAT integrated (`458688930`..`3b1be67dc`). Its checkout `mm7-monster-combat` is free again.

New lane:

| S-DEATH MM6 party death and respawn (M3) | `claude/mm6-party-death` in `mm7-monster-combat` | the detached MM6 party death model under `src/Engine/MM6/`, its MM6-gated hook where the party dies (`Party.cpp`, `Game.cpp` or `GameOver.cpp`, named in the handoff), new MM6 tests. Shared core. |

Update: S-BORDER part (c) is done on `claude/mm6-map-return` (`45ce7e223`, `c1db9506b`, `e5b12576f`), waiting for integration after the combat validation. New lane:

| S-SAVEANY MM6 save and load on any map (M2 part 2) | `claude/mm6-save-any-map` stacked on `claude/mm6-map-return` in `mm7-save-durability` | the MM6 EC6 writer and reader (`src/Engine/MM6/` save sections, outdoor and indoor visited-map sections), MM6 branches of `saveGame` and load in `src/Engine/SaveLoad.cpp`, the Save and Load menu admission for MM6, new MM6 tests and gate children. Shared core. |

Update: S-DEATH integrated (`8963ff45b`, `dad5aa244`). New lane in its checkout:

| S-HOVER MM6 decoration hover crash | `claude/mm6-decor-hover` in `mm7-monster-combat` | the MM6 decoration event-variable base and `GameUI_WritePointedObjectStatusString` MM6 path (`src/GUI/UI/UIGame.cpp` or wherever it lives), MM6 decoration setup in `src/Engine/MM6/` or `src/Engine/Graphics/Indoor.cpp` and `Outdoor.cpp` as needed, a new MM6 test. Shared core. |

Update: S-SAVEANY integrated (`5e752feab`..`0614e727b`). New lane in its checkout:

| S-TRAPS MM6 chest traps | `claude/mm6-chest-traps` in `mm7-save-durability` | the MM6 branch of chest opening and trap handling (`src/Engine/Objects/Chest.cpp` MM6 path, MM6 trap rules under `src/Engine/MM6/`), the disarm skill check, trap damage through the MM6 damage channels, new MM6 tests. Shared core. |

Update: S-HOVER and S-TRAPS integrated (`810cae956`, `404eacbd8`). A small follow-up lane re-pins four RNG-dependent combat tests on `claude/mm6-combat-tests-rng` in `mm7-monster-combat`. `mm7-save-durability` is free.

New lane:

| S-TRANSPORT MM6 boats and stables | `claude/mm6-transport` in `mm7-save-durability` | the MM6 transport house screen and routes (`src/GUI/UI/Houses/Transport.cpp` MM6 branch, `src/GUI/UI/Houses/MM6House.cpp`, MM6 route rules under `src/Engine/MM6/`), `leaveMap` on departure, new MM6 tests. Shared core. |

Update: S-TRANSPORT and the fix-prices step integrated (`251afc203`..`3c0a70b98`). New lane:

| S-QUICKSAVE MM6 autosave, quick save and in-game Load (M2/M3 remainder) | `claude/mm6-autosave-quickload` in `mm7-save-durability` | the MM6 slot policy for autosave and quick save names, the MM6 branches of `autoSave()`, quick save and quick load in `src/Engine/SaveLoad.cpp` and `src/Application/Game.cpp`, in-game Load admission in `MM6SaveLoadMenu.cpp`, new MM6 tests and gate children. Shared core. |

Update: S-QUICKSAVE integrated (`2a9f31413`..`657878857`). New lane, plus a read-only M4 chain planning pass:

| S-FINALE MM6 hive finale, winning and losing endings | `claude/mm6-finale` in `mm7-save-durability` | the MM6 game-over path replacing the MM7 one for houses 600/601 and the reactor destruction (`src/Application/GameOver.cpp`, `src/GUI/UI/UIGameOver.cpp` MM6 branches, MM6 finale code under `src/Engine/MM6/`), the hive losing-ending branch of party death, new MM6 tests. Shared core. |

New lanes for M4:

| H MM6 chain harness and C0 | `claude/mm6-chain-harness` in `mm7-monster-combat` | `test/Bin/GameTest/GameTestMain.cpp` (MM6 gate rows), `GameTestOptions.{h,cpp}`, new `MM6CampaignChain.{h,cpp}` and `GameTests_MM6CampaignChainC0.cpp`, `tools/run_mm7_campaign_chain.py` (title switch). Shared harness. |
| X MM6 house exits and the NPC 12 room chain | `claude/mm6-house-exits` in `mm7-load-admission` | `src/GUI/UI/Houses/MM6House.{h,cpp}`, `src/GUI/UI/MM6HouseServiceData.{h,cpp}`, new `src/Engine/MM6/MM6HouseExit.h`, new MM6 tests. |

