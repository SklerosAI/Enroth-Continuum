# Codex — MM7 saved NPC profession preflight

- Owner: Codex MM7 save reference audit lane
- Branch/worktree: `codex/mm7-audit-integration` in the existing `mm7-save-atomic` worktree
- Files owned: `src/Engine/MM7/MM7SaveValidation.{cpp,h}`, `src/Engine/MM7/Tests/MM7SaveValidation_ut.cpp`
- Purpose: reject out-of-range profession IDs on saved `NPC_HIRED` npcdata.bin rows while retaining unused rows and profession zero.
- Started: 2026-11-08
- Conflict check: main `1af5f586d`, clean branch, no active claim on validator files; native-root lane owns distinct startup/context files.
- Planned checks: synthetic pre-fix red, focused green, disposable MM7 writer/reader, network-denied throttled build, scoped style/diff.
- Handoff target: `agent_comms/handoffs/`
