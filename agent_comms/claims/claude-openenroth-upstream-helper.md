# Claude — upstream OpenEnroth playtest helper

- Owner: Claude Code lane subagent.
- Branch/worktree: `upstream-playtest` at upstream OpenEnroth master `8010ac364`, checkout `openenroth-upstream`. Upstream source is built as-is, with no edits.
- Files owned: nothing in this repository's history. Output is a staged `OpenEnroth.app` outside Git under the owner's Application Support staging folder, plus a refresh script and doc to move into `tools/` later.
- Purpose: the owner wants a second MM7 action in The Atlas that runs current upstream OpenEnroth for volunteer playtesting, with its own profile and the same recording and monitoring. The Atlas side is in `claims/claude-mm7-rolling-replay.md`.
- Started: 2026-11-10.
- Conflict check: no shared files. The upstream checkout uses the tenth worktree slot.
- Planned checks: upstream's own macOS build, version and CLI check without a window, ad-hoc signing check, size and disk report.
- Handoff target: Claude review, then the Atlas lane bundles it.
