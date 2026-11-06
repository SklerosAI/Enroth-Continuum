# Enroth Continuum agent notes

This repository is a fork of [OpenEnroth](https://github.com/OpenEnroth/OpenEnroth). Preserve its history, license, notices, and attribution. Read `CREDITS.md` and `project/UPSTREAM.md` before changing attribution.

## LLM wiki

The project uses a tracked `.llm-wiki/` based on the University of Notre Dame CRC `llm-wiki-memory-template`, adapted to the TDNet-style repository-local layout for Codex. Start with `.llm-wiki/index_Enroth-Continuum.md` when looking for durable project knowledge; follow `.llm-wiki/SCHEMA_Enroth-Continuum.md` when updating it. Treat current source and tests as authority over wiki summaries. Update relevant pages and the index after significant verified work, record superseded claims, and append a separately committed log entry. Routine fixes need no wiki entry.

For parallel work, read `.llm-wiki/Agent-Handoff.md` and run `git worktree list` before editing. The main `mm6-mac-implementation` checkout is the active integration tree; `mm7-isolated-runtime` is the protected MM7 baseline and `mm8-isolated-runtime` is reserved, not an implemented MM8 runtime. Coordinate file ownership and use separate branches/worktrees for concurrent code changes.

Never copy or upload private GOG installers, extracted game files, proprietary strategy-guide text, saves, backups, local logs, private mods, or detailed asset inventories to this public repository or wiki. `local-game-data/*` is ignored except its tracked README. Summarize private checks only at a high level and without redistributable content. See `.llm-wiki/Source-and-Privacy-Boundary.md`.

Do not treat an MM6 or MM8 title screen, parser probe, or partial map load as a playable campaign. Keep MM7, MM6, and MM8 runtimes and profiles separate; consult `project/LOCAL_STATUS.md` and the title checklists for current evidence.
