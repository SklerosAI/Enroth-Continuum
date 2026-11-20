# Enroth Continuum agent notes

This repository is a fork of [OpenEnroth](https://github.com/OpenEnroth/OpenEnroth). Preserve its history, license, notices, and attribution. Read `CREDITS.md` and `project/UPSTREAM.md` before changing attribution.

## LLM wiki

The project uses a tracked `.llm-wiki/` based on the University of Notre Dame CRC `llm-wiki-memory-template`, adapted to the TDNet-style repository-local layout for Codex. Start with `.llm-wiki/index_Enroth-Continuum.md` when looking for durable project knowledge; follow `.llm-wiki/SCHEMA_Enroth-Continuum.md` when updating it. Treat current source and tests as authority over wiki summaries. Update relevant pages and the index after significant verified work, record superseded claims, and append a separately committed log entry. Routine fixes need no wiki entry.

For parallel work, read `.llm-wiki/Agent-Handoff.md` and run `git worktree list` before editing. `master` is neutral integration; `codex/mm7` is the single continuing MM7 branch and `ECv0.1.0` is the fixed MM7 release tag. Use temporary branches/worktrees for concurrent MM7 changes, then consolidate verified work into `codex/mm7` and remove the temporary checkouts and refs at round end. `mm8-isolated-runtime` is reserved, not an implemented MM8 runtime. Coordinate file ownership and leave active Claude MM6 worktrees alone.

For a resumed MM6 task, start with `project/MM6_RESUME_HANDOFF.md`, the current checkpoint and token/time policy in `project/plan.md` section 18.1, and the top of `.llm-wiki/Agent-Handoff.md`. Then open only the source, tests and wiki packet for the named gate. The historical status and wiki are references, not a required full reread. Run focused checks for local edits and the full first-map chain at transaction milestones; use the MM7 control after shared-core changes. Limit concurrent code-changing agent lanes to two independent scopes.

Temporary parallel worktrees may exceed the old five-per-title and ten-total caps. Maintain at least 10 GB free disk space. At round end, keep one branch and one worktree per title; preserve uncommitted changes, local-only commits and ignored private files before retiring temporary checkouts. Keep branch refs until their work is merged or judged equivalent, then retire temporary refs; never merge an experimental branch merely to free space. Prefer Codex's worktree archive action for managed worktrees when it is available.

Never copy or upload private GOG installers, extracted game files, proprietary strategy-guide text, saves, backups, local logs, private mods, or detailed asset inventories to this public repository or wiki. `local-game-data/*` is ignored except its tracked README. Summarize private checks only at a high level and without redistributable content. See `.llm-wiki/Source-and-Privacy-Boundary.md`.

Do not treat an MM6 or MM8 title screen, parser probe, or partial map load as a playable campaign. Keep MM7, MM6, and MM8 runtimes and profiles separate; consult `project/LOCAL_STATUS.md` and the title checklists for current evidence.
