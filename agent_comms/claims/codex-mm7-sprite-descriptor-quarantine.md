# Codex — MM7 sprite descriptor quarantine

- Owner: Codex MM7 runtime lane.
- Branch/worktree: `codex/mm7-audit-integration` in `mm7-save-atomic`, based on integration `19eb2a4fa`.
- Retired source and test scope: `src/Engine/Objects/SpriteObject.cpp` and focused cases in `test/Bin/GameTest/GameTests_1000.cpp`.
- Purpose: clear only MM7 nonzero sprite descriptor IDs outside the loaded object table before map-load arrangement or object update indexes them. Keep free slots, valid IDs, and MM6 behavior unchanged.
- Serial GameTest lane: released after focused red/green and MM6 title control.
- Code/test commit: `4ae7bd3c4`; see `agent_comms/handoffs/2026-11-08-codex-mm7-sprite-descriptor-quarantine.md`.
- State: complete on isolated clean branch, awaiting integration review and combined gates. File ownership is released.
