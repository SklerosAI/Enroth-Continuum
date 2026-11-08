# Codex — MM7 energy attack parser

- Owner: Codex MM7 save reference audit lane
- Branch/worktree: `codex/mm7-audit-integration` in the existing `mm7-save-atomic` worktree
- Files owned: `src/Engine/Objects/Monsters.cpp`, `src/Engine/Objects/Tests/MonsterEnumFunctions_ut.cpp`
- Purpose: map MM7 `Ener` monster primary and secondary attacks to `DAMAGE_ENERGY`, preserving MM6's separate parser.
- Started: 2026-11-08
- Conflict check: main `1c95e0249` includes the MM7 combat handoff; the active Day of the Gods lane has a separate source/test scope.
- Planned checks: synthetic pre-fix red and green parser GameTests, MM6 parser control, scoped style/diff, network denied.
- Handoff target: `agent_comms/handoffs/`
