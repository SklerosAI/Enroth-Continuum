# Enroth Continuum agent notes

This repository is a fork of [OpenEnroth](https://github.com/OpenEnroth/OpenEnroth). Preserve its history, license, notices, and attribution. Read `CREDITS.md` and `project/UPSTREAM.md` before changing attribution.

## LLM wiki

The project uses a tracked `.llm-wiki/` based on the University of Notre Dame CRC `llm-wiki-memory-template`, adapted to the TDNet-style repository-local layout for Codex. Start with `.llm-wiki/index_Enroth-Continuum.md` when looking for durable project knowledge; follow `.llm-wiki/SCHEMA_Enroth-Continuum.md` when updating it. Treat current source and tests as authority over wiki summaries. Update relevant pages and the index after significant verified work, record superseded claims, and append a separately committed log entry. Routine fixes need no wiki entry.

For parallel work, read `.llm-wiki/Agent-Handoff.md` and run `git worktree list` before editing. `master` is neutral integration; `codex/mm7` is the single continuing MM7 branch and `ECv0.1.0` and `ECv0.1.1` are the fixed MM7 release tags. Use temporary branches/worktrees for concurrent MM7 changes, then consolidate verified work into `codex/mm7` and remove the temporary checkouts and refs at round end. `mm8-isolated-runtime` is reserved, not an implemented MM8 runtime. Coordinate file ownership and leave active Claude MM6 worktrees alone.

Claude Code owns MM6 on `claude/mm6`, whose curated `docs/` replaced the MM6 packets under `project/` and `.llm-wiki/` on 2026-11-19. For MM6 status start with `docs/mm6-campaign-status.md` and `docs/mm6-playthrough-go-no-go.md` on that branch. The MM6 files on this branch are historical. Run the MM7 controls after a shared-core change.

Temporary parallel worktrees may exceed the old five-per-title and ten-total caps. Maintain at least 10 GB free disk space. At round end, keep one branch and one worktree per title; preserve uncommitted changes, local-only commits and ignored private files before retiring temporary checkouts. Keep branch refs until their work is merged or judged equivalent, then retire temporary refs; never merge an experimental branch merely to free space. Prefer Codex's worktree archive action for managed worktrees when it is available.

## App replacement policy

Unless the owner explicitly says otherwise, every installed-app rebuild replaces
the prior Enroth Continuum app completely. Delete superseded installed bundles
and staged app artifacts after the replacement is verified. Do not keep an old
app version as a rollback copy: repair or migrate saved games for the current
build when necessary. This policy does not authorize deleting profiles or saves
outside an explicitly scoped repair or cleanup task.

Never copy or upload private GOG installers, extracted game files, proprietary strategy-guide text, saves, backups, local logs, private mods, or detailed asset inventories to this public repository or wiki. `local-game-data/*` is ignored except its tracked README. Summarize private checks only at a high level and without redistributable content. See `.llm-wiki/Source-and-Privacy-Boundary.md`.

Do not treat an MM6 or MM8 title screen, parser probe, or partial map load as a playable campaign. Keep MM7, MM6, and MM8 runtimes and profiles separate; consult `project/LOCAL_STATUS.md` and the title checklists for current evidence.
