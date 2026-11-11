# Claude — MM7 rolling replay recorder

- Owner: Claude Code lane subagent. Taken over from Codex's paused, uncommitted work at the owner's request on 2026-11-10.
- Branch/worktree: `claude/mm7-rolling-replay` from `master`, new checkout `mm7-rolling-replay`. Codex's edits are carried unchanged in `5e3d2cf3c`.
- Files owned: `launcher/AtlasApp.swift`, `launcher/Atlas/index.html`, `launcher/README.md`, `launcher/assemble.py`, `tools/mm7_playtest_capture.sh`, `tools/MM7_PLAYTEST_CAPTURE.md`, and launcher test scripts if any.
- Purpose: the owner accepts continuous capture but wants at most the last 2 minutes kept on disk. Command-Shift-R saves a clip of the last 2 minutes. A crash or abnormal MM7 exit keeps the last 2 minutes. A normal exit discards the buffer.
- Started: 2026-11-10.
- Conflict check: no other lane touches the launcher or recorder.
- Planned checks: synthetic recorder tests, Swift build, staged Atlas assembled around the already-installed MM7 helper, signature checks. Install only after Claude review, keeping a rollback copy.
- Handoff target: Claude review, then install and an owner check of Command-Shift-R.
