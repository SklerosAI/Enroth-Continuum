# MM6 monster description, sprite and spawn links — F2

Status: **retained-data/source audit, 2026-11-05**. This packet is an offline join, not a running encounter or an original-file F3 result. Exact names, map placements, hashes and per-ID details are in ignored `local-game-data/mm6/research-2026-11-05/monster-spawn-links-probe.json`.

## Method and coverage

`scripts/mm6_probe_monster_spawn_links.py` reads only retained MM6 extracts: 173 `monsters.txt` rows, 173 fixed 148-byte `dmonlist.bin` descriptions, the `dsft.bin` frame/index table, and 67 decoded `.dlv`/`.ddm` map deltas. It joins description names to text-table picture tokens after case/space normalization; joins each nonempty animation name to the extended sprite-frame name index; and counts baseline creature records by the candidate monster ID at byte offset 52 of each 548-byte record. That offset comes from the pinned MM6 `Creature.java` in [MM8LevelEditor](https://github.com/angeld29/mm8leveleditor/blob/bf3b8c4a86100635a5fc6ff53fac6e4e0af5cb53/src/org/gamenet/application/mm8leveleditor/data/mm6/Creature.java). It remains an external schema hypothesis until F3/original behavior corroborates it.

The retained-data join finds 173/173 normalized description/table-name matches and 1,382/1,384 nonempty animation-name matches in 1,656 extended sprite names (6,455 frame records). The two misses occupy one animation slot for two IDs. It decodes 342 baseline creature records in 67 deltas, with 24 distinct candidate monster IDs. One record carries candidate ID 173. This checks names and indices; it does not decode texture pixels, prove animation playback, or cover dynamic/scripted spawns.

## Source mismatch and confidence

The MM6 path in `src/Engine/Snapshots/TableSerialization.cpp` reads exactly 173 `MonsterDesc_MM6` records, then calls `monsters.pop_back()` and fills `MonsterList` IDs 1–172. The final retained description is a real named row in the retained table, and the candidate actor-ID join places one baseline record at 173. `Actor::PrepareSprites` in `src/Engine/Objects/Actor.cpp` indexes `pMonsterList->monsters[monsterInfo.id]` and passes its animation names to `SpriteFrameTable::FastFindSprite` in `src/Engine/Graphics/Sprites.cpp`. Thus the code/data join identifies a likely missing description at ID 173. It does **not** prove a live crash, blank sprite, or original MM6 rendering. The fixed `MonsterList` ID bounds, default entry behavior and actual map-load path must be checked before changing the loader.

The two guide-catalog tail IDs are outside the printed 57-group monster catalog, but that does not establish that either shipped ID is unused. The candidate placement specifically challenges the source comment that the final MM6 entry is unused.

## F3, F4 and F5 handoff

- **F3 original preflight:** compare original hashes and bytes for the three joined tables and affected map delta with the retained snapshot; verify 173 descriptor names, animation-name index ordering and the actor ID offset/width against original executable/schema evidence. Preserve exact placements privately.
- **F4 offline build:** make MM6's description loader and `MonsterList` capacity/ID policy cover every active ID, including 173, without changing MM7's loader. Add a bounded all-ID name/sprite lookup test and explicit behavior for absent animation names. Check the actual texture/frame loading path and scripted spawn IDs before locking the monster implementation.
- **F5 graphical play:** enter the affected map and representative dynamic-spawn encounters, inspect the visible monster and animation states, exercise combat, then save/reload and recheck placement. Record screenshots and exact map/ID evidence only in ignored local storage.

This packet addresses identity, descriptor and baseline spawn linkage only. Damage, AI, factions, resistance, loot and full guide reconciliation remain in [combat semantics](MM6_COMBAT_SEMANTICS_F2.md) and [monster guide semantics](MM6_MONSTER_GUIDE_SEMANTICS_F2.md). F2 remains open.
