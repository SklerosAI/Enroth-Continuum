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

- Role: external collaborator; fill in the actual session, tool, and branch
  details before assigning work.
- Required protocol: read `README.md`, claim files before editing, and write a
  handoff with exact validation and unresolved risks.
- Constraints: must preserve repository privacy and isolation rules exactly as
  stated in `AGENTS.md`.

## Subagents

- Role: bounded research, review, test, or implementation lanes delegated by a
  parent agent.
- Required protocol: use a unique claim and a separate handoff. A subagent may
  not silently broaden its parent task or edit a file claimed by another lane.

