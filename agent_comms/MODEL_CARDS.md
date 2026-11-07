# Model cards

Update a card when an agent's tools, constraints, or role materially change.
Keep credentials, private paths, and machine-specific secrets out of cards.

## Codex

- Role: implementation, focused testing, integration review, and repository
  coordination.
- Strengths: inspect Git state and worktrees; make scoped edits; run builds and
  tests; review committed or working-tree diffs.
- Communication: reads and writes this directory, Git commits, and approved
  project handoff documents.
- Constraints: does not have a direct message channel to an external Claude
  session. It learns Claude's work through shared files, commits, diffs, or a
  user-provided bridge.

## Claude

- Role: MM6 lead and integrator (set 2026-11-07). Owns the MM6 plan, file
  ownership, lane integration into `mm6-mac-implementation`, and the F4b
  Save/Load and play gates. Assigns bounded MM6 tasks to Codex through
  `messages/`.
- Session: Claude Code in VS Code on this Mac, working in the main
  integration checkout. Spawns MM6 lane subagents in separate worktrees under
  `~/.codex/worktrees/mm6-*`. Lane state is in
  `project/SESSION_HANDOFF_2026-11-07.md`.
- Communication: polls and writes `agent_comms/live/` at the canonical main
  checkout path for immediate notes, commits `messages/`, `claims/` and
  `handoffs/` for durable ones.
- Required protocol: read `README.md`, claim files before editing, and write a
  handoff with exact validation and unresolved risks.
- Constraints: must preserve repository privacy and isolation rules exactly as
  stated in `AGENTS.md`.

## Subagents

- Role: bounded research, review, test, or implementation lanes delegated by a
  parent agent.
- Required protocol: use a unique claim and a separate handoff. A subagent may
  not silently broaden its parent task or edit a file claimed by another lane.

