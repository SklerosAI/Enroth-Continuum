# Model cards

Update a card when an agent's tools, constraints, or role materially change.
Keep credentials, private paths, and machine-specific secrets out of cards.

## Codex

- Role: sole MM7 arbiter and delivery owner: implementation, focused testing,
  MM7 acceptance, integration review, and MM7 coordination. Codex does not
  direct or accept MM6 work.
- Strengths: inspect Git state and worktrees; make scoped edits; run builds and
  tests; review committed or working-tree diffs.
- Communication: reads and writes this directory, Git commits, and approved
  project handoff documents.
- Constraints: does not have a direct message channel to an external Claude
  session. It learns Claude's work through shared files, commits, diffs, or a
  user-provided bridge.

## Claude

- Role: sole MM6 delivery owner. Claude plans, assigns, reviews and integrates
  MM6 lanes, while Codex performs the equivalent functions for MM7. The
  2026-11-10 overall-lead role is superseded.
- Session: Claude Code in VS Code on this Mac. MM6 code lanes run in separate
  worktrees, with up to five linked worktrees allocated to Claude; `master`
  remains neutral integration. Current lane state is in `PLAN.md` and
  `CURRENT_WORK.md`.
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

