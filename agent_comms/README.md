# Agent communications

This directory is the durable, repository-local coordination point for Claude,
Codex, and their subagents. It supplements the project wiki; source and tests
remain authoritative.

## How to use it

1. Read `CURRENT_WORK.md` and the relevant model card before starting.
2. Claim a narrow, non-overlapping file scope in `claims/` before editing.
3. Record a handoff in `handoffs/` when pausing, completing, or transferring
   work. Include the commit, files owned, checks run, remaining blocker, and
   privacy note.
4. Put review results in `reviews/`; do not overwrite another agent's handoff.
5. Use `messages/` for a direct asynchronous question or reply, then copy any
   durable decision into a handoff or claim.
6. Commit coordination updates separately from code changes when practical.

For concurrent worktrees, commit the handoff or claim and have the receiving
agent fetch/rebase before relying on it. A file in one worktree is not live
communication with another worktree until it is shared through Git. Do not
use the main checkout as a shared write location for concurrent source edits.

## Boundaries

- Never place GOG installers, extracted game data, saves, private mods,
  screenshots, raw logs, checksums of private files, or copied guide text here.
- Use high-level validation summaries only.
- Do not claim a file already owned by an active lane. Resolve overlap by a
  handoff or explicit owner decision before editing.
- A model card describes capabilities and constraints; it is not authority to
  change project scope or bypass `AGENTS.md`.

## Layout

- `MODEL_CARDS.md` — capabilities and operating constraints for each agent.
- `CURRENT_WORK.md` — short shared task board.
- `PLAN.md` — the coordination lead's living plan and gate rows.
- `claims/` — one active claim per lane, named `<agent>-<scope>.md`.
- `handoffs/` — append-only task handoffs, named with ISO date and scope.
- `reviews/` — review notes tied to a commit or branch.
- `messages/` — Git-mediated direct messages between named agents.

