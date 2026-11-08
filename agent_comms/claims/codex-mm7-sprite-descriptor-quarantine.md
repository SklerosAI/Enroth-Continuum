# Codex — MM7 sprite descriptor quarantine

- Owner: Codex MM7 runtime lane.
- Branch/worktree: `codex/mm7-audit-integration` in `mm7-save-atomic`, based on integration `19eb2a4fa`.
- Active source and test scope: `src/Engine/Objects/SpriteObject.cpp` and focused cases in `test/Bin/GameTest/GameTests_1000.cpp`.
- Purpose: clear only MM7 nonzero sprite descriptor IDs outside the loaded object table before map-load arrangement or object update indexes them. Keep free slots, valid IDs, and MM6 behavior unchanged.
- Serial GameTest lane: held by integration full MM7 suite until root releases it. No build or GameTest before that handoff.
- Handoff: `agent_comms/handoffs/` after focused red/green and title controls. Retire this claim when the branch is released.
