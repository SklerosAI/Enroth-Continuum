# MM7 chain chapters C1-C3 handoff

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm7-chain-c1-c3` in the `mm7-load-admission` checkout, on top of the integrated harness (`b915d9d4d` there, `3d6e81da8` on `master`). Commits `f52ce6006`, `bca37af44`, `548d41d94`, `db458c215`, `9d464d55e`. Cherry-pick these five in order. Not pushed.
- Files: new `test/Bin/GameTest/MM7CampaignChain.{h,cpp}` (shared chapter helper), `GameTests_MM7CampaignChainC1.cpp` to `C3.cpp`, CMake lines, and the resume rule in `tools/run_mm7_campaign_chain.py`.

## Chapters

- C1, save000 to save001: castle door and Butler, goblins killed (aiState fixture, as `MM7CastlePath`), leaving the castle sets the goblin bit, tavern repair request. Tapes pin every bit, NPC 58's house, the award and the experience gained.
- C2, save001 to save002: Stone City king, elixir, the seven Red Dwarf Mines statues, rebuild hand-in. Tapes pin bits, items, gold, experience, NPC houses and the hired-dwarf mask.
- C3, save002 to save003: the castle timer on reload, 112 rests in the castle (NoActors only for the rest loop), the messenger on the Harmondale reload, the Bracada judge, the arbiter's Light choice.
- No quest-bit or NPC-field writes at any chapter start. Teleports between maps are named in each chapter's first comment.

## Runner

Resume trusts only pass markers. An unmarked chapter removes its own slot, backup and marker and those of every later chapter, then reruns from there. The chain helper waits for the Load window instead of a fixed tick count. A one-tick wait was a null pointer in the test helper, not an engine crash.

## Checks

- Fresh `--through 3`: exit 0, four slots and markers. A rerun says already passed. Deleting C2's marker removes save002 and save003, reruns C2 and C3, exit 0, saves byte-identical to the first run.
- Behavior guards: skipping the tavern hand-in, the seventh statue or one rest fails that chapter with no save or marker.
- Stray files and a symlinked save still refuse without removing anything.
- In-memory `MM7CastlePath`, `MM7CastleRebuild`, `MM7Campaign`, `MM7LightWalls` 10/10. Legacy `MM7CastleRebuild` and `MM7LightWalls` pairs pass. cpplint clean. Full headless MM7 binary 533 passed, 2 skipped, before the two follow-up commits, which touch only the runner and the helper.

Privacy: no game data, saves or logs in Git.
