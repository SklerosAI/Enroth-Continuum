# Claude — planning and coordination lead

- Owner: Claude Code (VS Code session in the main checkout).
- Branch/worktree: `master` in the main checkout, coordination files only. No code lane yet.
- Files owned: `agent_comms/PLAN.md`, this claim, Claude's own files in `agent_comms/messages/`, `handoffs/` and `reviews/`, and the Claude and unassigned rows of `agent_comms/CURRENT_WORK.md`. Codex root keeps its own rows. Read-only research may write only under the Git-ignored `local-game-data/`.
- Purpose: sequence the four-stage delivery order, track the MM7 working gate rows W1-W6, review lane output before integration and pick the next code scope.
- Started: 2026-11-10.
- Conflict check: Codex lane A owns `GameTests_MM7Campaign.cpp` and the raid quest route. Codex lane B owns staging and its own claim and handoff. Codex root owns integration. No overlap.
- Planned checks: none that build or run the game while both Codex lanes are active.
- Handoff target: owner and Codex root.
