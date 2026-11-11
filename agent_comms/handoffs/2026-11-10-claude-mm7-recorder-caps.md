# MM7 recorder storage caps handoff

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm7-recorder-caps` in the `mm7-rolling-replay` checkout, from `master` `eb57d6ee6`. Commits `3a62be76a`, `80681e60d`, `6fd2ee2a7`. Not pushed.
- The installed Atlas was rebuilt from this branch and replaced in place, with no backup copy (owner rule). Both helpers are byte-identical to the previous install. Integrate soon so `master` matches what is installed.

## Caps

- At most 3 copies per save file per session (`--max-save-snapshots` or `EC_MM7_MAX_SAVE_SNAPSHOTS`). Older copies are deleted, and their index lines are marked pruned.
- Scheduled checkpoints copy a save only when its content changed.
- At most 20 recording sessions kept (`--max-sessions` or `EC_MM7_MAX_SESSIONS`). A session is pruned only if it is a direct child of the destination on the same volume, carries the exact recorder marker, holds no clip or crash report, and is not a running session whose game is alive. Deletion never follows symlinks or crosses volumes, and the marker goes last so an interrupted removal retries.
- Protected sessions beyond the cap and recordings over 2 GiB raise storage warnings in `status.json` and in the message Atlas shows when the game closes. They are never deleted.

## Checks

`tools/tests/test_mm7_playtest_capture.py` 26/26 (8 new, which fail against the previous recorder). Atlas compiles, the Swift launcher checks and `launcher/tests/atlas_page.js` pass. The installed bundle passes a strict deep signature check and matches the staged one, and the bundled recorder matches the committed script.

## Not yet verified

Real gameplay, a real recording folder and an external drive. The end-of-game message in the GUI. Screen Recording permission must likely be granted again after this reinstall.

Privacy: no video, saves, logs or game data in Git.
