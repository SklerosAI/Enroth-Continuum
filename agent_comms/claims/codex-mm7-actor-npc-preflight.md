# Codex — MM7 actor NPC preflight

- Owner: Codex MM7 save reference audit lane
- Branch/worktree: `codex/mm7-audit-integration` in the existing `mm7-save-atomic` worktree
- Files owned: `src/Engine/MM7/MM7SaveValidation.{cpp,h}`, `src/Engine/MM7/Tests/MM7SaveValidation_ut.cpp`
- Purpose: reject provably invalid nonnegative actor NPC IDs in detached indoor and outdoor MM7 deltas.
- Started: 2026-11-08
- Conflict check: main `9bccea648` contains the prior Energy handoff; current Day of the Gods work uses distinct spell files and tests. No other active claim covers MM7SaveValidation.
- Planned checks: synthetic pre-fix red, bounded green unit tests, fresh disposable MM7 writer/reader if needed, scoped style and diff.
- Handoff target: `agent_comms/handoffs/`
