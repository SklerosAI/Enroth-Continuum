# Codex — MM7 actor group dialogue bounds

- Owner: Codex MM7 save reference lane
- Branch/worktree: `codex/mm7-audit-integration` in the existing `mm7-save-atomic` worktree
- Files owned: `src/Engine/Interaction.cpp` and focused tests in `test/Bin/GameTest/GameTests_1000.cpp`
- Purpose: bound actor group and NPC news phrase indices at the live interaction read, without rejecting saved group values used for other actor behavior
- Started: 2026-11-08 UTC
- Conflict check: no active claim or uncommitted change for either file in the integration or MM7 combat worktree; MM6 Lane L owns SaveLoad and UI files, which are outside this scope
- Planned checks: isolated pre-fix failing MM7 cases, network-denied throttled GameTest build, focused MM7 checks and MM6 control, scoped style and diff checks
- Handoff target: `agent_comms/handoffs/`
