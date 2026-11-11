# Claude — recorder storage caps

- Owner: Claude Code lane subagent.
- Branch/worktree: `claude/mm7-recorder-caps` from `master`, in the `mm7-rolling-replay` checkout.
- Files owned: `tools/mm7_playtest_capture.sh`, `tools/tests/test_mm7_playtest_capture.py`, `tools/MM7_PLAYTEST_CAPTURE.md`, and `launcher/AtlasApp.swift`, `launcher/Atlas/index.html` and `launcher/README.md` only if the caps need a setting or a message.
- Purpose: the owner asked that saves never stack without a cap. Diagnostics copied every autosave change and every scheduled checkpoint into each session, and sessions accumulated. Cap save snapshots per source file, skip unchanged checkpoints, and cap kept sessions without deleting requested clips. Then rebuild and replace the installed Atlas in place, keeping no old copy.
- Started: 2026-11-10.
- Conflict check: no other lane touches the launcher or recorder.
- Planned checks: synthetic recorder tests, Swift build, assembled bundle signature and an unchanged MM7 helper hash, install without a backup.
- Handoff target: Claude review.
