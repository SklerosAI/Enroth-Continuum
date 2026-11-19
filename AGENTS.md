# Enroth Continuum agent notes

This repository is a fork of [OpenEnroth](https://github.com/OpenEnroth/OpenEnroth). Preserve its history, license, notices, and attribution. Read `CREDITS.md` and `docs/upstream.md` before changing attribution.

## Docs

`docs/` is the project's knowledge base, built on the University of Notre Dame CRC `llm-wiki-memory-template` pattern. Start with `docs/README.md` and `docs/index.md` when looking for durable project knowledge, and follow `docs/SCHEMA.md` when updating it. Treat current source and tests as authority over docs pages. Update the page that owns a fact in the same change that alters it, and add a line to `docs/log.md` for a dated milestone. Routine fixes need no docs entry. Reasoning about a change goes in its commit message, not in a docs page or a handoff file.

For parallel work, read `docs/agent-workflow.md` and run `git worktree list` before editing. `master` is neutral integration. `codex/mm7` is the single continuing MM7 branch and `ECv0.1.0` is the fixed MM7 release tag. MM6 lanes integrate into `claude/mm6`. Use temporary branches and worktrees for concurrent changes, then consolidate verified work into the title's branch and remove the temporary checkouts and refs at round end. `mm8-isolated-runtime` is reserved, not an implemented MM8 runtime. Coordinate file ownership through lane branch names and the local `agent_comms/live/` mailbox, and leave active Claude MM6 worktrees alone.

For a resumed task, start with the title's status page, `docs/mm7-status.md` or `docs/mm6-campaign-status.md`, and `docs/open-questions.md`. Then open only the source, tests and docs page for the named gate. Docs pages are references, not a required full reread. Run focused checks for local edits and the full chain at transaction milestones. Use the MM7 controls after shared-core changes, see `docs/testing.md`.

Temporary parallel worktrees may exceed the old five-per-title and ten-total caps. Maintain at least 10 GB free disk space. At round end, keep one branch and one worktree per title. Preserve uncommitted changes, local-only commits and ignored private files before retiring temporary checkouts. Keep branch refs until their work is merged or judged equivalent, then retire temporary refs. Never merge an experimental branch merely to free space. Prefer Codex's worktree archive action for managed worktrees when it is available.

Never copy or upload private GOG installers, extracted game files, proprietary strategy-guide text, saves, backups, local logs, private mods, or detailed asset inventories to this public repository or its docs. `local-game-data/*` is ignored except its tracked README. Summarize private checks only at a high level and without redistributable content. See `docs/source-and-privacy.md`.

Do not treat an MM6 or MM8 title screen, parser probe, or partial map load as a playable campaign. Keep MM7, MM6, and MM8 runtimes and profiles separate. Consult `docs/mm7-status.md`, `docs/mm6-campaign-status.md` and `docs/project-overview.md` for current evidence.
