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

